#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleDefinition.hh"
#include "G4GenericMessenger.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

#include <vector>

namespace B1
{

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:

    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Commands from macro
    void SetParticle(const G4String& name);
    void SetIon(G4int Z, G4int A);
    void SetSpectrum(const G4String& filename);
    void SetDirection(const G4String& direction);

private:

    // Sampling
    G4double SampleEnergy();
    G4ThreeVector SampleCosineDirection();
    G4ThreeVector SampleSourcePosition();

    void LoadSpectrum(const G4String& filename);

    // Particle gun
    G4ParticleGun* fParticleGun;

    // Messenger
    G4GenericMessenger* fMessenger;

    // Particle configuration
    G4ParticleDefinition* fParticle;
    G4String fParticleName;

    // Source configuration
    G4String fDirection;

    G4double fSourceRadius;
    G4double fDownwardSourceZ;
    G4double fUpwardSourceZ;

    // Spectrum
    std::vector<G4double> fEnergyBins;
    std::vector<G4double> fEnergyProbs;
};

}
