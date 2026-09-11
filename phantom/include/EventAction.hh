/// \file B1/include/EventAction.hh
/// \brief Definition of the B1::EventAction class

#ifndef B1EventAction_h
#define B1EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "OrganDosimetry.hh"

#include <map>
#include <string>

namespace B1
{

class RunAction;

/// Event action class

class EventAction : public G4UserEventAction
{
  public:
    EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // F6 Tally: Dose absorvida
    void AddEdep(G4double edep) { fEdep += edep; }

    // F6+DF Tally: Dose equivalente
    void AddEquivalentDose(G4double dose) { fEquivalentDose += dose; }

    // Dose por tipo de partícula
    void AddEdepByParticle(G4String particle, G4double edep) {
      fEdepByParticle[particle] += edep;
    }

    // F2 Tally: Fluxo
    void AddFlux(G4String direction, G4String particle, G4double energy);

    // Getters
    G4double GetEdep() const { return fEdep; }
    G4double GetEquivalentDose() const { return fEquivalentDose; }
    std::map<G4String, G4double> GetEdepByParticle() const { return fEdepByParticle; }




  private:
    RunAction* fRunAction = nullptr;
    G4double   fEdep = 0.;
    G4double fEquivalentDose = 0;
    std::map<G4String, G4double> fEdepByParticle;

    // Estrutura para fluxo: direction -> particle -> count
    std::map<G4String, std::map<G4String, G4int>> fFluxCount;
    std::map<G4String, std::map<G4String, G4double>> fFluxEnergy;

  };

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif


