/// \file B1/include/PrimaryGeneratorAction.hh
/// \brief Definition of the B1::PrimaryGeneratorAction class

#ifndef B1PrimaryGeneratorAction_h
#define B1PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;

namespace B1
{

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    // method from the base class
    void GeneratePrimaries(G4Event*) override;

    // method to access particle gun
    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }

  private:
    G4ParticleGun* fParticleGun;

    G4ParticleDefinition* fIon;

    // Parâmetros da fonte em disco
    G4double fSourceZ;

    // Vetores e arrays para o espectro de energia
    std::vector<G4double> fEnergyBins;
    std::vector<G4double> fEnergyProbs;

    void InitializeEnergySpectrum();
    G4double SampleEnergy();
    G4ThreeVector SampleCosineDirection();

};

}

#endif
