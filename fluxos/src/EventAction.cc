
/// \file B1/src/EventAction.cc
/// \brief Implementation of the B1::EventAction class

#include "EventAction.hh"
#include "RunAction.hh"
#include "Run.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"

#include "G4SystemOfUnits.hh"

namespace B1
{

EventAction::EventAction()//RunAction* runAction)
: G4UserEventAction(),
  //fRunAction(runAction),
  fEdep(0.),
  fEquivalentDose(0.)
{}


void EventAction::BeginOfEventAction(const G4Event*)
{
  fEdep = 0.;
  fEquivalentDose = 0.;
  fEdepByParticle.clear();
  fFluxCount.clear();
  fFluxEnergy.clear();
}


//void EventAction::AddFlux(G4String direction,
  //                        G4String particle,
    //                      G4double energy)
//{
  //fFluxCount[direction][particle]++;
  //fFluxEnergy[direction][particle] += energy;
//}


void EventAction::EndOfEventAction(const G4Event* event)
{

  // Get the thread-local Run object (our custom Run class)
  auto* run = static_cast<Run*>(
    G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  if (!run) return;

  // Push scalar accumulators
  run->AddEdep(fEdep);
  run->AddEdep2(fEdep * fEdep);
  run->AddEquivalentDose(fEquivalentDose);
  run->AddEquivalentDose2(fEquivalentDose * fEquivalentDose);

  // Push dose by particle
  for (const auto& pair : fEdepByParticle) {
    run->AddEdepByParticle(pair.first, pair.second);
  }

  // Push flux counts and energies
  for (const auto& dirPair : fFluxCount) {
    const G4String& direction = dirPair.first;
    for (const auto& partPair : dirPair.second) {
      const G4String& particle = partPair.first;
      G4int count = partPair.second;
      G4double energy = fFluxEnergy[direction][particle];
      run->AddFluxCount(direction, particle, count);
      run->AddFluxEnergy(direction, particle, energy);

  //if (fRunAction) {
    //fRunAction->AddEdep(fEdep);
    //fRunAction->AddEquivalentDose(fEquivalentDose);

    // Passar dose por partícula
    //for (const auto& pair : fEdepByParticle) {
      //fRunAction->AddEdepByParticle(pair.first, pair.second);
    //}

    // Passar fluxos
    //for (const auto& dirPair : fFluxCount) {
      //G4String direction = dirPair.first;
      //for (const auto& partPair : dirPair.second) {
        //G4String particle = partPair.first;
        //G4int count = partPair.second;
        //G4double energy = fFluxEnergy[direction][particle];

        //fRunAction->AddFlux(direction, particle, energy);
    }

  }

 }

}
