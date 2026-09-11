/// \file B1/src/ActionInitialization.cc
/// \brief Implementation of the B1::ActionInitialization class

#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "G4RunManager.hh"

namespace B1
{

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void ActionInitialization::Build() const
{
  SetUserAction(new PrimaryGeneratorAction);

  auto runAction = new RunAction;
  SetUserAction(runAction);

  auto det = static_cast<const DetectorConstruction*>(
    G4RunManager::GetRunManager()->GetUserDetectorConstruction()
  );

  auto eventAction = new EventAction(runAction);
  SetUserAction(eventAction);

  SetUserAction(new SteppingAction(eventAction));

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
