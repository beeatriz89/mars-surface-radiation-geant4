/// \file B1/src/EventAction.cc
/// \brief Implementation of the B1::EventAction class

#include "EventAction.hh"
#include "RunAction.hh"
#include "OrganDosimetry.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"

#include "G4SystemOfUnits.hh"

namespace B1
{

EventAction::EventAction(RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::BeginOfEventAction(const G4Event*)
{
}

void EventAction::EndOfEventAction(const G4Event* event)
{

    // Flush per-step accumulators into per-event statistics
    auto* runAction = static_cast<RunAction*>(
        const_cast<G4UserRunAction*>(
            G4RunManager::GetRunManager()->GetUserRunAction()));
    if (runAction && runAction->GetDosimetry())
        runAction->GetDosimetry()->EndOfEvent();

}

}
