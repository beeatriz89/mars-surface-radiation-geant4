// test_stopping_power.cc
//
// Programa standalone, minimo: inicializa a physics list, constroi as
// tabelas de fisica (RunManager::Initialize), e depois chama
// G4EmCalculator::GetDEDX() DIRETAMENTE, sem correr nenhum evento, sem
// nenhum stepping, sem nenhuma reacao nuclear no meio.
//
// Objetivo: isolar se a tabela de stopping power (a mesma que o
// SteppingAction usa) esta correta para o alfa, sem a "contaminacao"
// de reacoes nucleares que vimos no LET_doseAvg do scoring completo.

#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4ParticleGun.hh"
#include "G4Event.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

#include "FTFP_INCLXX_HP.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4IonINCLXXPhysics.hh"

#include "G4EmCalculator.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"

#include <vector>
#include <iomanip>
#include <iostream>

// Detector minimo, so para termos um material valido (nem precisa de
// ser fisicamente sensato - o GetDEDX so precisa do G4Material)
class MinimalDetector : public G4VUserDetectorConstruction
{
  public:
    G4VPhysicalVolume* Construct() override
    {
      G4NistManager* nist = G4NistManager::Instance();
      G4Material* tissue = nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
      G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

      G4double worldHalf = 1.0 * m;
      auto solidWorld = new G4Box("World", worldHalf, worldHalf, worldHalf);
      auto logicWorld = new G4LogicalVolume(solidWorld, vacuum, "World");
      auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);

      // um pequeno bloco de tecido, so para o material existir na geometria
      auto solidTissue = new G4Box("Tissue", 1 * cm, 1 * cm, 1 * cm);
      auto logicTissue = new G4LogicalVolume(solidTissue, tissue, "Tissue");
      new G4PVPlacement(nullptr, G4ThreeVector(), logicTissue, "Tissue", logicWorld, false, 0);

      fTissueMaterial = tissue;
      return physWorld;
    }

    G4Material* GetTissueMaterial() const { return fTissueMaterial; }

  private:
    G4Material* fTissueMaterial = nullptr;
};

// Gun minima: so precisamos de UM evento fictício para forcar o Geant4
// a construir por completo as tabelas de cortes de producao/couples
// (isto NAO gera dados usados no teste - so "acorda" as tabelas internas)
class MinimalGenerator : public G4VUserPrimaryGeneratorAction
{
  public:
    MinimalGenerator()
    {
      fGun = new G4ParticleGun(1);
      fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
      fGun->SetParticleEnergy(1.0 * keV);
      fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
      fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    }
    ~MinimalGenerator() override { delete fGun; }
    void GeneratePrimaries(G4Event* event) override { fGun->GeneratePrimaryVertex(event); }

  private:
    G4ParticleGun* fGun;
};

class MinimalActionInitialization : public G4VUserActionInitialization
{
  public:
    void Build() const override { SetUserAction(new MinimalGenerator()); }
};

int main()
{
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

  auto* detector = new MinimalDetector();
  runManager->SetUserInitialization(detector);

  // MESMA physics list da producao, para o teste ser comparavel
  auto physicsList = new FTFP_INCLXX_HP();
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option3());
  physicsList->ReplacePhysics(new G4IonINCLXXPhysics());
  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new MinimalActionInitialization());

  // Constroi geometria + tabelas de fisica
  runManager->Initialize();

  // BeamOn(1) fictício: forca a construcao COMPLETA das tabelas de
  // cortes de producao / couples (Initialize() sozinho por vezes nao
  // chega). O evento em si (1 gamma de 1 keV) e irrelevante - so serve
  // para desencadear a inicializacao interna completa.
  runManager->BeamOn(1);

  G4Material* tissue = detector->GetTissueMaterial();

  G4EmCalculator calc;
  G4ParticleTable* table = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* proton = table->FindParticle("proton");
  G4ParticleDefinition* alpha = G4IonTable::GetIonTable()->GetIon(2, 4, 0.0);

  // Energias por nucleao (MeV/n) a testar - inclui a gama onde vimos o desvio
  std::vector<G4double> energies_per_n = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000};

  std::cout << "\n=== G4EmCalculator::GetDEDX direto (SEM transporte/reacoes) ===\n";
  std::cout << std::left
             << std::setw(14) << "E [MeV/n]"
             << std::setw(18) << "LET_proton[keV/um]"
             << std::setw(18) << "LET_alpha[keV/um]"
             << std::setw(12) << "razao(esp~4)"
             << "\n";

  for (G4double e_per_n : energies_per_n) {
    G4double kinE_p = e_per_n * MeV;         // protao: A=1
    G4double kinE_a = e_per_n * 4.0 * MeV;   // alfa: A=4, mesma velocidade -> 4x energia total

    G4double dedx_p = calc.GetDEDX(kinE_p, proton, tissue);
    G4double dedx_a = calc.GetDEDX(kinE_a, alpha, tissue);

    G4double let_p = dedx_p / (MeV / mm);  // MeV/mm == keV/um numericamente
    G4double let_a = dedx_a / (MeV / mm);

    std::cout << std::left << std::fixed << std::setprecision(4)
               << std::setw(14) << e_per_n
               << std::setw(18) << let_p
               << std::setw(18) << let_a
               << std::setw(12) << (let_p > 0 ? let_a / let_p : 0.0)
               << "\n";
  }

  delete runManager;
  return 0;
}
