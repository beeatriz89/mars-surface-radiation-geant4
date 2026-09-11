#pragma once
#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1
#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include "DetectorConstruction.hh"
#include "G4Step.hh"
#include <map>
#include <vector>
#include "G4ThreeVector.hh"

class G4LogicalVolume;
class TETModelImport;

namespace B1
{

class EventAction;
class RunAction;
class DetectorConstruction;

class SteppingAction : public G4UserSteppingAction
{
  public:

    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction();

    //SteppingAction(const DetectorConstruction*, RunAction*);
    //~SteppingAction() override;

    void UserSteppingAction(const G4Step*) override;

  private:
    void CacheTetData();
    EventAction* fEventAction;
    DetectorConstruction* fDetConstruction;
    RunAction* fRunAction;
    TETModelImport* fTetData;

  };

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
