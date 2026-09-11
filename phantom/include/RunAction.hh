#pragma once
/// RunAction.hh  —  Mars Dosimetry Project

#include "G4UserRunAction.hh"
#include "globals.hh"

class OrganDosimetry;
class G4Run;

namespace B1
{

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    ~RunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction  (const G4Run*) override;

    OrganDosimetry* GetDosimetry();

private:
    OrganDosimetry* fDosimetry;   // thread-local accumulator (owned)
};

} // namespace B1
