/// \file exampleB1.cc
/// \brief Main program of the B1 example

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UImanager.hh"
#include "QBBC.hh"
#include "FTFP_BERT.hh"

#include "QGSP_BERT_HP.hh"

// Physics headers
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4HadronPhysicsINCLXX.hh"
#include "G4IonINCLXXPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "FTFP_INCLXX_HP.hh"
#include "FTFP_BERT_HP.hh"

#include "G4SystemOfUnits.hh"

#include "G4ProductionCutsTable.hh"

#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "Randomize.hh"

#include "G4Proton.hh"

#include "G4ScoringManager.hh"

using namespace B1;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{

  G4String macroFile;
  G4String outputFile;
  G4bool isAF = false;

  for (int i = 1; i < argc; i++)
  {
    G4String arg = argv[i];

    if (arg == "-m" && i + 1 < argc)
    {
        macroFile = argv[++i];
    }
    else if (arg == "-o" && i + 1 < argc)
    {
        outputFile = argv[++i];
    }
    else if (arg == "-f")
    {
        isAF = true;
    }
    else if (arg[0] != '-')
    {
        // compatibilidade com o teu formato antigo:
        macroFile = arg;
    }
  }

  G4cout << "[MAIN] Program started" << G4endl;

  // Detect interactive mode (if no arguments) and define UI session
  G4UIExecutive* ui = nullptr;
  //if (argc == 1) {
    //  G4cout << "[MAIN] Starting UI session" << G4endl;
    //  ui = new G4UIExecutive(argc, argv);
  //}

  if (macroFile.empty())
  {
      G4cout << "[MAIN] Starting UI session" << G4endl;
      ui = new G4UIExecutive(argc, argv);
  }

  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);


  // ============================================================
  // CRITICAL FIX: Creating the ScoreWriter before the RunManager
  // ============================================================

  G4ScoringManager* scoringManager = G4ScoringManager::GetScoringManager();
  scoringManager->SetVerboseLevel(1);

  // ============================================================


  G4cout << "[MAIN] Creating RunManager" << G4endl;
  auto runManager =
    G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Detector construction
  G4cout << "[MAIN] Setting DetectorConstruction" << G4endl;
  runManager->SetUserInitialization(new DetectorConstruction(isAF));


  // Physics list
  G4cout << "[MAIN] Setting PhysicsList" << G4endl;

  auto physicsList = new FTFP_INCLXX_HP();

  // Electromagnetic physics (optimized option3)
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option3());

  // Ion physics: INCL++ for heavy ion fragmentation (critical for GCR)
  physicsList->ReplacePhysics(new G4IonINCLXXPhysics());

  physicsList->SetVerboseLevel(0);

  /////////////////////////////////////////////////////

  // Production cuts (distância, não energia direta)
  // Geant4 usa "range cuts" ao invés de energy cuts
  physicsList->SetDefaultCutValue(0.01*mm);

  // Cuts específicos por partícula (opcional)
  physicsList->SetCutValue(0.01*mm, "gamma");
  physicsList->SetCutValue(0.01*mm, "e-");
  physicsList->SetCutValue(0.01*mm, "e+");

  runManager->SetUserInitialization(physicsList);

  // User action initialization
  G4cout << "[MAIN] Setting ActionInitialization" << G4endl;
  runManager->SetUserInitialization(new ActionInitialization());

  // VIS
  G4cout << "[MAIN] Initializing visualization" << G4endl;
  auto visManager = new G4VisExecutive(argc, argv);
  visManager->Initialize();

  auto UImanager = G4UImanager::GetUIpointer();

  // Process macro OR UI
  if (!ui) {
      G4cout << "[MAIN] Batch mode: executing macro " << argv[1] << G4endl;

      G4String command = "/control/execute ";
      //G4String fileName = argv[1];
      G4String fileName = macroFile;


      UImanager->ApplyCommand(command + fileName);

      //auto table = G4ProductionCutsTable::GetProductionCutsTable();

      //table->DumpCouples();

      G4cout << "[MAIN] Macro finished execution." << G4endl;
  } else {
      G4cout << "[MAIN] Interactive mode: executing init_vis.mac" << G4endl;

      UImanager->ApplyCommand("/control/execute init_vis.mac");

      G4cout << "[MAIN] Starting UI session..." << G4endl;
      ui->SessionStart();

      G4cout << "[MAIN] UI session ended" << G4endl;
      delete ui;
  }

  G4cout << "[MAIN] Cleaning up..." << G4endl;
  delete visManager;
  delete runManager;

  G4cout << "[MAIN] Program terminated normally." << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
