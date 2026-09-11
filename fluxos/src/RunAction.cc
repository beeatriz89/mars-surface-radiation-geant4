/// \file B1/src/RunAction.cc
/// \brief Implementation of the B1::RunAction class

#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "Run.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <iomanip>
#include "G4Threading.hh"
#include <cmath>

#include "RADSensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include <fstream>

namespace B1
{

RunAction::RunAction()
: G4UserRunAction()
{
  G4cout << "[RunAction] Created, this=" << this
         << " thread=" << G4Threading::G4GetThreadId() << G4endl;
}

G4Run* RunAction::GenerateRun()
{
  return new Run;
}


void RunAction::BeginOfRunAction(const G4Run*)
{

  G4cout << "[BeginOfRunAction] thread=" << G4Threading::G4GetThreadId() << G4endl;

  const auto detConstruction = static_cast<const DetectorConstruction*>(
    G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  fScoringVolume = detConstruction->GetScoringVolume();
}

G4double GetQualityFactor(const G4String& name)
{
    if (name == "gamma") return 1.0;
    if (name == "mu-") return 1.0;
    if (name == "pi-") return 2.0;
    if (name == "e-") return 1.0;
    if (name == "mu+") return 1.0;
    if (name == "pi+") return 2.0;
    if (name == "e+") return 1.0;
    if (name == "proton") return 2.0;
    if (name == "2H") return 2.0;
    if (name == "3H") return 2.0;
    if (name == "3He") return 20.0;
    if (name == "alpha") return 20.0;
    if (G4StrUtil::contains(name,"ion")) return 20.0;
    if (name == "neutron") return 10.0;
    return 1.0;
}

void RunAction::WriteFluxSpectra(const G4Run* aRun)
{

  const auto* run = static_cast<const Run*>(aRun);
  const auto& allSpectra = run->GetFluxSpectra();
  const auto& edges = Run::EnergyBinEdges();

  G4cout << "\n=== Writing Flux Spectra to Files ===" << G4endl;

  const std::vector<G4String> directions = {"downward", "upward", "RAD_cone", "4pi"};

  for (const auto& direction : directions) {
    auto dirIt = allSpectra.find(direction);
    if (dirIt == allSpectra.end()) continue;

    for (const auto& particlePair : dirIt->second) {
      const G4String& particle = particlePair.first;
      const auto& h = particlePair.second;
      if (h.nTotal == 0) continue;

      const G4String filename = "flux_spectrum_" + direction + "_" + particle + ".dat";
      std::ofstream outfile(filename);
      if (!outfile.is_open()) {
        G4cout << "  ERROR: Could not open " << filename << G4endl;
        continue;
      }

      outfile << "# Flux spectrum: " << direction << ", particle: " << particle << "\n";
      outfile << "# E_min[MeV]  E_max[MeV]  E_center[MeV]  WeightedCount  Entries  WeightedSqCount\n";
      outfile << "# Total crossings scored: " << h.nTotal << "\n";
      outfile << "# Underflow (E < "  << Run::kEminMeV << " MeV): " << h.nUnder << "\n";
      outfile << "# Overflow  (E >= " << Run::kEmaxMeV << " MeV): " << h.nOver  << "\n";
      outfile << "# WEIGHTED by 1/cos(theta) - Matthia method\n";
      outfile << "# Post-process with normalize_flux.py for physical units\n";

      for (G4int i = 0; i < Run::kNBins; ++i) {
        const G4double Emin_bin = edges[i]   / MeV;
        const G4double Emax_bin = edges[i+1] / MeV;
        const G4double Ecenter  = std::sqrt(Emin_bin * Emax_bin);

        outfile << std::scientific << std::setprecision(6)
                << Emin_bin << "  " << Emax_bin << "  " << Ecenter << "  "
                << h.weight[i] << "  "
                << h.count[i] << "  "
                << h.weightSq[i] << "\n";
      }
      outfile.close();
      G4cout << "  Written: " << filename
             << " (" << h.nTotal << " crossings)" << G4endl;
    }
  }

}

void RunAction::EndOfRunAction(const G4Run* aRun)
{

  const auto* run = static_cast<const Run*>(aRun);

  G4int nEvents = run->GetNumberOfEvent();
  if (nEvents == 0) return;

  // Only print results and write files on the master thread
  if (!IsMaster()) return;

  G4cout << "\n============ END OF RUN (MASTER, all threads merged) ============" << G4endl;
  G4cout << "Total events: " << nEvents << G4endl;


  G4double density = 1.0 * g/cm3;  // soft tissue
  G4double volume = 71.56940763959247 * cm3;
  G4double mass = density * volume;
  G4double mass_kg = mass / kg;

  G4double edepTotal = run->GetEdep();
  G4double edep2Total = run->GetEdep2();
  G4double edep_MeV = edepTotal / MeV;
  G4double dose_MeV_per_kg = edep_MeV / mass_kg;
  G4double dose_Gy = dose_MeV_per_kg * 1.602176634e-13;

  G4double edep_avg = edepTotal / nEvents;
  G4double edep2_avg = edep2Total / nEvents;
  G4double variance = edep2_avg - edep_avg * edep_avg;
  G4double rms = (variance > 0) ? std::sqrt(variance) : 0.0;

  // ========== Write flux spectra (ALL threads merged) ==========
  WriteFluxSpectra(aRun);

  G4cout << "\n" << std::string(70, '=') << G4endl;


}


}

