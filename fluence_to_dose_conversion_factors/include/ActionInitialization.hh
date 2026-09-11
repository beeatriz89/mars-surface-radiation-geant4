#ifndef ActionInitialization_h
#define ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

class DetectorConstruction;

class ActionInitialization : public G4VUserActionInitialization
{
  public:
    explicit ActionInitialization(const DetectorConstruction* detector);
    ~ActionInitialization() override = default;

    void BuildForMaster() const override; // master: só RunAction (para merge/print)
    void Build() const override;          // workers: gun + run + stepping

  private:
    const DetectorConstruction* fDetector = nullptr;
};

#endif
