#include "EventAction.hh"
#include "RunAction.hh"
#include "G4Event.hh"

EventAction::EventAction(RunAction* runAction)
  : fRunAction(runAction)
{}

void EventAction::BeginOfEventAction(const G4Event*)
{
  fEventDose = 0.0;
  fEventWeightedDose = 0.0;
  fEventLETDose = 0.0;
}

void EventAction::EndOfEventAction(const G4Event*)
{
  // Envia o total deste evento para o RunAction (uma amostra independente,
  // usada para estimar a incerteza estatística da média por-primário)
  fRunAction->AddEventSample(fEventDose, fEventWeightedDose, fEventLETDose);
}
