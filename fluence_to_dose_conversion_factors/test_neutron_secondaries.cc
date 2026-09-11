// test_neutron_secondaries.cc
//
// Programa standalone: dispara neutroes contra a placa de tecido e
// reporta a dose ACUMULADA POR ESPECIE DE SECUNDARIO (protao, alfa,
// gama, nucleos pesados, etc). Objetivo: confirmar se os canais de
// captura (n,p) em N-14 e (n,alfa) em O-16 - conhecidos por serem
// grandes contribuidores ao H de neutroes em tecido - estao mesmo a
// aparecer na simulacao, e em que proporcao.
//
// Serial (nao MT) de proposito - e so um diagnostico, nao precisa de
// paralelismo, e assim evitamos ter de fazer merge de mapas entre
// threads.
//
// Adiciona ao CMakeLists.txt:
//   add_executable(testNeutronSecondaries test_neutron_secondaries.cc)
//   target_link_libraries(testNeutronSecondaries ${Geant4_LIBRARIES})
//
// Uso: ./testNeutronSecondaries <energia_MeV> <N_eventos>
//   ex: ./testNeutronSecondaries 100 200000

#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4ParticleGun.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleTable.hh"

#include "FTFP_INCLXX_HP.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4IonINCLXXPhysics.hh"

#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <cstdlib>

namespace {
  // Estado global simples (programa serial, um so thread - sem problema)
  std::map<G4String, G4double> gDoseBySpecies;
  G4double gTotalDose = 0.0;
  G4LogicalVolume* gSlabLogical = nullptr;
  G4double gSlabMass = 0.0;
  G4double gEnergyMeV = 100.0;
}

class MinimalDetector : public G4VUserDetectorConstruction
{
  public:
    G4VPhysicalVolume* Construct() override
    {
      G4NistManager* nist = G4NistManager::Instance();
      G4Material* tissue = nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRU-4");
      G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

      G4double worldHalf = 1.0 * m;
      auto solidWorld = new G4Box("World", worldHalf, worldHalf, worldHalf);
      auto logicWorld = new G4LogicalVolume(solidWorld, vacuum, "World");
      auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);

      // Placa 30x30cm, 0.3mm de espessura (mesma geometria da producao atual)
      G4double halfX = 15.0 * cm, halfY = 15.0 * cm, halfZ = 0.15 * mm;
      auto solidSlab = new G4Box("Slab", halfX, halfY, halfZ);
      auto logicSlab = new G4LogicalVolume(solidSlab, tissue, "Slab");
      new G4PVPlacement(nullptr, G4ThreeVector(), logicSlab, "Slab", logicWorld, false, 0);

      gSlabLogical = logicSlab;
      gSlabMass = logicSlab->GetMass();
      return physWorld;
    }
};

class NeutronGenerator : public G4VUserPrimaryGeneratorAction
{
  public:
    NeutronGenerator()
    {
      fGun = new G4ParticleGun(1);
      fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
      fGun->SetParticleEnergy(gEnergyMeV * MeV);
      fGun->SetParticlePosition(G4ThreeVector(0, 0, 1 * cm));
      fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    }
    ~NeutronGenerator() override { delete fGun; }
    void GeneratePrimaries(G4Event* event) override { fGun->GeneratePrimaryVertex(event); }

  private:
    G4ParticleGun* fGun;
};

class SpeciesSteppingAction : public G4UserSteppingAction
{
  public:
    void UserSteppingAction(const G4Step* step) override
    {
      G4LogicalVolume* volume =
          step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
      if (volume != gSlabLogical) return;

      G4double edep = step->GetTotalEnergyDeposit();
      if (edep <= 0.0) return;

      G4String name = step->GetTrack()->GetDefinition()->GetParticleName();
      gDoseBySpecies[name] += edep;
      gTotalDose += edep;
    }
};

class DiagnosticActionInitialization : public G4VUserActionInitialization
{
  public:
    void Build() const override
    {
      SetUserAction(new NeutronGenerator());
      SetUserAction(new SpeciesSteppingAction());
    }
};

int main(int argc, char** argv)
{
  gEnergyMeV = (argc > 1) ? std::atof(argv[1]) : 100.0;
  G4int nEvents = (argc > 2) ? std::atoi(argv[2]) : 200000;

  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

  runManager->SetUserInitialization(new MinimalDetector());

  auto physicsList = new FTFP_INCLXX_HP();
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option3());
  physicsList->ReplacePhysics(new G4IonINCLXXPhysics());
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new DiagnosticActionInitialization());

  runManager->Initialize();

  std::cout << "\nA disparar " << nEvents << " neutroes a " << gEnergyMeV << " MeV...\n";
  runManager->BeamOn(nEvents);

  std::cout << "\n=== DOSE POR ESPECIE DE SECUNDARIO (neutrao a " << gEnergyMeV << " MeV) ===\n";
  std::cout << std::left << std::setw(15) << "Especie"
             << std::setw(18) << "Edep_total[MeV]"
             << std::setw(12) << "% do total"
             << "\n";

  // Ordenar por contribuicao decrescente
  std::vector<std::pair<G4String, G4double>> sorted(gDoseBySpecies.begin(), gDoseBySpecies.end());
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  for (const auto& [name, edep] : sorted) {
    G4double pct = (gTotalDose > 0) ? 100.0 * edep / gTotalDose : 0.0;
    std::cout << std::left << std::setw(15) << name
               << std::setw(18) << std::fixed << std::setprecision(4) << edep
               << std::setw(12) << std::setprecision(2) << pct
               << "\n";
  }

  std::cout << "\nEdep total: " << gTotalDose << " MeV\n";
  std::cout << "\n>>> Procura por 'proton' e 'alpha' na lista acima (canais (n,p) e (n,alfa)).\n";
  std::cout << ">>> Se a percentagem deles parecer baixa demais (ex: <10-15% combinados,\n";
  std::cout << ">>> dependendo da energia), pode confirmar dados HP em falta/incompletos.\n";

  delete runManager;
  return 0;
}
