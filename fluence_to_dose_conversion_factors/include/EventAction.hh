#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class RunAction;

// Agrega, para UM evento, as contribuições de todos os passos dentro
// da placa (podem ser vários: partícula primária + secundários).
// No fim do evento, envia o total desse evento para o RunAction, que
// usa esses valores por-evento para estimar a incerteza estatística
// (cada evento é uma amostra independente).
class EventAction : public G4UserEventAction
{
  public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

    // Chamados pelo SteppingAction, um ou mais vezes por evento
    void AddStepDose(G4double doseStep)            { fEventDose += doseStep; }
    void AddStepWeightedDose(G4double weightedStep) { fEventWeightedDose += weightedStep; }
    void AddStepLETDose(G4double letDoseStep)       { fEventLETDose += letDoseStep; }

  private:
    RunAction* fRunAction = nullptr;

    G4double fEventDose = 0.0;
    G4double fEventWeightedDose = 0.0;
    G4double fEventLETDose = 0.0;
};

#endif
