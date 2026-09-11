#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "FTFP_INCLXX_HP.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4IonINCLXXPhysics.hh"
#include "FTFP_BERT_HP.hh"

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

int main(int argc, char** argv)
{
  // G4RunManagerFactory escolhe MT automaticamente se o Geant4 tiver
  // sido compilado com suporte a multithreading; caso contrário cai
  // para sequencial sem precisares de mudar nada aqui.
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Nº de threads (só tem efeito em builds MT); podes também definir
  // via macro com /run/numberOfThreads N antes do /run/initialize
  runManager->SetNumberOfThreads(8);

  auto* detector = new DetectorConstruction();
  runManager->SetUserInitialization(detector);

  // Physics list: FTFP_INCLXX_HP (INCL++ para fragmentação de iões
  // pesados, importante para GCR) + EM option3 (mais precisa) +
  // G4IonINCLXXPhysics para a física de iões
  auto physicsList = new FTFP_INCLXX_HP();
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option3());
  physicsList->ReplacePhysics(new G4IonINCLXXPhysics());
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new ActionInitialization(detector));

  G4UImanager* uiManager = G4UImanager::GetUIpointer();

  if (argc > 1) {
    // Modo batch: root executa a macro passada em argv[1]
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    uiManager->ApplyCommand(command + fileName);
  } else {
    // Modo interativo (opcional, útil para visualização/debug)
    auto* ui = new G4UIExecutive(argc, argv);
    auto* visManager = new G4VisExecutive();
    visManager->Initialize();
    uiManager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
    delete visManager;
  }

  delete runManager;
  return 0;
}
