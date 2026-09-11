/// \file B1/include/SteppingAction.hh
/// \brief Definition of the B1::SteppingAction class

#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include "DetectorConstruction.hh"
#include "G4ParticleDefinition.hh"
#include <map>
#include <vector>

#include "G4ThreeVector.hh"

class G4LogicalVolume;

namespace B1
{

class EventAction;
class RunAction;
class DetectorConstruction;

/// Stepping action class

class SteppingAction : public G4UserSteppingAction
{
  public:

    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction();

    void UserSteppingAction(const G4Step*) override;

  private:
    EventAction* fEventAction = nullptr;
    G4LogicalVolume* fScoringVolume = nullptr;

    G4double fScoringZ;
    G4double fSurfaceTolerance; // Tolerância para detetar crossing

    G4bool IsCrossingSurface(const G4Step* step, G4double zSurface, G4bool& isDownward);
    G4String GetParticleCategory(const G4ParticleDefinition* pdef);
    G4double GetQualityFactor(const G4String& particleName, G4double energy);

  };

}

#endif
