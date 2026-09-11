#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class DetectorConstruction;
class G4Run;
class G4GenericMessenger;

class RunAction : public G4UserRunAction
{
  public:
    explicit RunAction(const DetectorConstruction* detector);
    ~RunAction() override;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

    // Chamado pelo EventAction no fim de cada evento (uma amostra
    // independente); os accumulables tratam do merge entre threads
    void AddEventSample(G4double doseEvt, G4double weightedDoseEvt, G4double letDoseEvt);

    void SetCsvFileName(G4String name); // ligado a /output/csvFile
    void SetPrimaryOnly(G4bool enabled); // ligado a /scoring/primaryOnly

  private:
    const DetectorConstruction* fDetector = nullptr;
    G4GenericMessenger* fMessenger = nullptr;

    G4Accumulable<G4double> fDose{"Dose", 0.};                 // sum_i dose_i
    G4Accumulable<G4double> fDoseSq{"DoseSq", 0.};              // sum_i dose_i^2
    G4Accumulable<G4double> fWeightedDose{"WeightedDose", 0.};  // sum_i H_i  (H = Q.D)
    G4Accumulable<G4double> fWeightedDoseSq{"WeightedDoseSq", 0.};
    G4Accumulable<G4double> fLETDose{"LETDose", 0.};            // sum_i LET_i . dose_i
};

#endif
