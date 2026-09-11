#ifndef B1StackingAction_h
#define B1StackingAction_h 1

#include "G4UserStackingAction.hh"
#include "globals.hh"

namespace B1
{

class StackingAction : public G4UserStackingAction
{
  public:
    StackingAction() = default;
    ~StackingAction() override = default;

    G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;
};

}
#endif
