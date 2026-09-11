#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization(const DetectorConstruction* detector)
  : fDetector(detector)
{}

void ActionInitialization::BuildForMaster() const
{
  // O master não gera eventos (não precisa de PrimaryGeneratorAction),
  // mas precisa de um RunAction para fazer o Merge() dos accumulables
  // vindos das workers e imprimir/gravar o resultado final.
  SetUserAction(new RunAction(fDetector));
}

void ActionInitialization::Build() const
{
  auto generator  = new PrimaryGeneratorAction(fDetector);
  auto runAction  = new RunAction(fDetector);
  auto eventAction = new EventAction(runAction);

  SetUserAction(generator);
  SetUserAction(runAction);
  SetUserAction(eventAction);
  SetUserAction(new SteppingAction(fDetector, eventAction));
}
