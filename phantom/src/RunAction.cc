#include "RunAction.hh"
#include "OrganDosimetry.hh"
#include "DetectorConstruction.hh"
#include "TETDetectorConstruction.hh"
#include "TETModelImport.hh"
#include "G4ProductionCutsTable.hh"

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4Run.hh"
#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"

#include <iomanip>
#include <sstream>
#include <cmath>

// ---- Static master accumulator + mutex -----------------------------------
namespace {
    G4Mutex          gDosimetryMutex  = G4MUTEX_INITIALIZER;
    OrganDosimetry*  gMasterDosimetry = nullptr;
}

namespace B1
{

// ---------------------------------------------------------------------------
RunAction::RunAction()
: G4UserRunAction(), fDosimetry(new OrganDosimetry())
{
    G4cout << "[RunAction] created, thread="
           << G4Threading::G4GetThreadId()
           << "  this=" << this << G4endl;
}

RunAction::~RunAction()
{
    delete fDosimetry;
}

OrganDosimetry* RunAction::GetDosimetry() { return fDosimetry; }

// ---------------------------------------------------------------------------
void RunAction::BeginOfRunAction(const G4Run*)
{
    G4cout << "[BeginOfRunAction] thread="
           << G4Threading::G4GetThreadId() << G4endl;

    fDosimetry->Reset();

    if (G4Threading::IsMasterThread()) {
        G4AutoLock lock(&gDosimetryMutex);
        delete gMasterDosimetry;
        gMasterDosimetry = new OrganDosimetry();
    }
}

// ---------------------------------------------------------------------------
void RunAction::EndOfRunAction(const G4Run* run)
{
    // ---- Worker: merge into master ----------------------------------------
    if (!G4Threading::IsMasterThread()) {
        G4AutoLock lock(&gDosimetryMutex);
        if (gMasterDosimetry)
            gMasterDosimetry->Merge(*fDosimetry);
        G4cout << "[EndOfRunAction] worker thread="
               << G4Threading::G4GetThreadId()
               << " merged." << G4endl;
        return;
    }

    // ---- Master: write outputs --------------------------------------------
    G4cout << "\n[EndOfRunAction] MASTER — writing dosimetry outputs...\n" << G4endl;

    if (!gMasterDosimetry) {
        G4cerr << "[RunAction] ERROR: gMasterDosimetry is null." << G4endl;
        return;
    }

    // ---- Get TETModelImport* — masses and names come directly from here --
    // No external OrganMasses.dat needed — TETModelImport calculates
    // mass = density * volume for every organ from the .node/.ele/.material files.
    auto* dc = static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    const TETModelImport* tetData = nullptr;
    if (dc && dc->GetPhantom())
        tetData = dc->GetPhantom()->GetTetData();

    if (!tetData) {
        G4cerr << "[RunAction] ERROR: cannot retrieve TETModelImport." << G4endl;
        return;
    }

    const auto& massMap      = tetData->GetMassMap();
    const auto& organNameMap = tetData->GetOrganNameMap();

    // Find max matID to size vectors
    G4int maxID = 0;
    for (const auto& [id, mass] : massMap)
        if (id > maxID) maxID = id;

    G4cout << "[RunAction] Max organ matID = " << maxID
           << "  Number of organs = " << massMap.size() << G4endl;

    std::vector<G4double> organMasses_g(maxID + 1, 0.);
    std::vector<G4String> organNames(maxID + 1, "unknown");

    for (const auto& [id, mass] : massMap)
        organMasses_g[id] = mass / g;   // G4 internal units → grams

    for (const auto& [id, name] : organNameMap)
        organNames[id] = name;

    // Debug: print first 8 organs
    G4cout << "[RunAction] Sample organ masses:" << G4endl;
    G4int shown = 0;
    for (const auto& [id, mass] : massMap) {
        if (shown++ >= 8) break;
        G4cout << "   matID=" << std::setw(5) << id
               << "  mass=" << std::setw(10) << std::fixed << std::setprecision(3)
               << mass/g << " g"
               << "  name=" << organNameMap.at(id) << G4endl;
    }

    // ---- Write output files -----------------------------------------------
    const G4String prefix     = "MarsDosimetry";
    G4int          nPrimaries = run->GetNumberOfEvent();
    G4cout << "[RunAction] N primaries = " << nPrimaries << G4endl;

    gMasterDosimetry->WriteOutputs(organMasses_g, organNames, prefix, nPrimaries);

    G4cout << "\n[RunAction] Output files written:\n"
           << "  → " << prefix << "_absorbed.out\n"
           << "  → " << prefix << "_equivalent.out\n"
           << "  → " << prefix << "_effective.out\n" << G4endl;

    G4cout << "\nRun finished successfully.\n" << G4endl;
}

} // namespace B1
