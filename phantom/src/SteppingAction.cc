/// SteppingAction.cc  —  Mars Dosimetry Project
///
/// SCORING STRATEGY (ICRP-145 / MRCP_AF):
///
///   The phantom is a G4PVParameterised volume named "wholePhantom".
///   Each tetrahedron has a copy number; TETParameterisation::ComputeMaterial()
///   calls fTetData->GetMaterialIndex(copyNo) to assign material.
///   The material's *name* comes directly from the *.material file
///   (e.g. "RBM", "liver", "lung", ...).
///
///   We identify the scoring voxel by its logical volume name "TetLogic"
///   (set in TETDetectorConstruction::ConstructPhantom()).
///   The organ ID (matID) is the G4PVParameterised copy number's material index,
///   which is exactly the mXX id from the *.material file.
///   We pass that directly to OrganDosimetry::Accumulate() — no string lookup needed.
///
///   The copy number → matID mapping is owned by TETModelImport and is
///   retrieved via GetMaterialIndex(copyNo).  We cache the pointer to
///   TETModelImport once per run for performance.

#include "SteppingAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "OrganDosimetry.hh"

// Geant4
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleDefinition.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4VPhysicalVolume.hh"
#include "G4TouchableHistory.hh"
#include "G4Navigator.hh"

// TET phantom
#include "TETModelImport.hh"
#include "TETDetectorConstruction.hh"
#include "DetectorConstruction.hh"

#include <cmath>

namespace B1
{

// ---------------------------------------------------------------------------
SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fRunAction(nullptr),
  fTetData(nullptr)
{}

SteppingAction::~SteppingAction() {}

// ---------------------------------------------------------------------------
// CacheTetData — called once at the first step that needs it.
// Retrieves TETModelImport* from the detector construction.
// ---------------------------------------------------------------------------
void SteppingAction::CacheTetData()
{
    if (fTetData) return;  // already cached

    auto* dc = static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    if (!dc) return;

    // DetectorConstruction owns fPhantom (TETDetectorConstruction*)
    const TETDetectorConstruction* tetDC = dc->GetPhantom();
    if (!tetDC) return;

    fTetData = tetDC->GetTetData();

    G4cout << "[SteppingAction] TETModelImport cached: "
           << (fTetData ? "OK" : "FAILED") << G4endl;
}

// ---------------------------------------------------------------------------
void SteppingAction::UserSteppingAction(const G4Step* step)
{


    static G4int nSteps = 0;

    if (nSteps < 50) {
        auto* pv = step->GetPreStepPoint()
                       ->GetTouchableHandle()
                       ->GetVolume();

      if (pv) {
            G4cout
            << "[STEP DEBUG] "
            << "particle = "
            << step->GetTrack()->GetDefinition()->GetParticleName()
            << " | volume = "
            << pv->GetName()
            << " | logical = "
            << pv->GetLogicalVolume()->GetName()
            << " | pos = "
            << step->GetPreStepPoint()->GetPosition() / m
            << " m"
            << " | edep = "
            << step->GetTotalEnergyDeposit() / MeV
            << " MeV"
            << G4endl;
        }

      nSteps++;
}


    static G4int debugSteps = 0;
    // ---- Energy deposition guard ------------------------------------------
    G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.) return;

    // ---- Identify the volume ----------------------------------------------
    // The TET phantom parameterised volume uses logical volume "TetLogic"
    // as set in TETDetectorConstruction::ConstructPhantom().
    const G4TouchableHandle& touch = step->GetPreStepPoint()->GetTouchableHandle();
    G4LogicalVolume* lv = touch->GetVolume()->GetLogicalVolume();
    if (!lv) {
        if (debugSteps < 20) {
            G4cout << "[DEBUG] edep > 0 but logical volume = NULL"
                   << G4endl;
            debugSteps++;
        }
        return;
     }

      if (debugSteps < 20) {
        G4cout
            << "\n========== EDEP DEBUG ==========\n"
            << "Edep        = " << edep / MeV << " MeV\n"
            << "Logical     = " << lv->GetName() << "\n"
            << "Copy number = " << touch->GetCopyNumber() << "\n"
            << "Replica     = " << touch->GetReplicaNumber(0) << "\n"
            << "Position    = "
            << step->GetPreStepPoint()->GetPosition() / m
            << " m\n"
            << "Particle    = "
            << step->GetTrack()->GetDefinition()->GetParticleName()
            << "\n"
            << "=================================\n"
            << G4endl;

        debugSteps++;
    }

    // Accept only steps inside the tetrahedral phantom
    if (lv->GetName() != "TetLogic") return;

    // ---- Get TETModelImport pointer (cached) ------------------------------
    CacheTetData();
    if (!fTetData) return;

    // ---- Map copy number → organ material ID (matID) ----------------------
    // The physical volume inside a G4PVParameterised is accessed via
    // GetReplicaNumber() at depth 0 of the touchable.
    G4int copyNo = touch->GetReplicaNumber(0);
    G4int orgID  = fTetData->GetMaterialIndex(copyNo);
    if (orgID < 0) return;   // should not happen, but guard

    // ---- Particle info ----------------------------------------------------
    const G4Track*              track = step->GetTrack();
    const G4ParticleDefinition* pd    = track->GetDefinition();
    const G4String&             pname = pd->GetParticleName();
    G4int pZ = pd->GetAtomicNumber();
    G4int pA = pd->GetAtomicMass();

    // ---- Step geometry ----------------------------------------------------
    G4double stepLen_mm     = step->GetStepLength() / mm;
    G4double density_g_cm3  = lv->GetMaterial()->GetDensity() / (g/cm3);

    // ---- Direction (downward = primary GCR, upward = secondary/albedo) ----
    G4ThreeVector momDir = step->GetPreStepPoint()->GetMomentumDirection();

    // ---- Get thread-local RunAction → OrganDosimetry ----------------------
    auto* runAction = static_cast<RunAction*>(
        const_cast<G4UserRunAction*>(
            G4RunManager::GetRunManager()->GetUserRunAction()));
    if (!runAction) return;

    OrganDosimetry* dosimetry = runAction->GetDosimetry();
    if (!dosimetry) return;

    static G4int debugAccumulate = 0;

    G4StepPoint* preP  = step->GetPreStepPoint();
    G4StepPoint* postP = step->GetPostStepPoint();
    G4double Ek_pre  = preP->GetKineticEnergy();
    G4double Ek_post = postP->GetKineticEnergy();
    G4double Ek_mean = 0.5 * (Ek_pre + Ek_post);

    if (debugAccumulate < 20) {
        G4cout
        << "\n******** ACCUMULATE ********\n"
        << "orgID       = " << orgID << "\n"
        << "particle    = " << pname << "\n"
        << "edep        = " << edep / MeV << " MeV\n"
        << "stepLength  = " << stepLen_mm << " mm\n"
        << "density     = " << density_g_cm3 << " g/cm3\n"
        << "kinetic E   = "
        << track->GetKineticEnergy() / MeV << " MeV\n"
        << "*****************************\n"
        << G4endl;

        debugAccumulate++;
}

    // ---- Accumulate -------------------------------------------------------
    dosimetry->Accumulate(orgID,
                          pd, pZ, pA,
                          Ek_mean / MeV,
                          edep,           // G4 internal units (MeV)
                          stepLen_mm,
                          density_g_cm3,
                          momDir);
}

}
