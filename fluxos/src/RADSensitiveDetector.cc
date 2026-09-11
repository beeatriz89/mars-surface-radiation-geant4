#include "RADSensitiveDetector.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4EmCalculator.hh"
#include "G4SystemOfUnits.hh"

RADSensitiveDetector::RADSensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name)
{}

G4bool RADSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4double stepLength = step->GetStepLength();
    if (stepLength <= 0.) return false;

    G4Track* track = step->GetTrack();
    const G4ParticleDefinition* particle =
        track->GetParticleDefinition();

    G4double energy =
        step->GetPreStepPoint()->GetKineticEnergy();

    const G4Material* material =
        step->GetPreStepPoint()->GetMaterial();

    static G4EmCalculator emCalc;

    G4double dedx =
        emCalc.ComputeTotalDEDX(energy, particle, material);

    G4double energyLoss = dedx * stepLength;

    G4String name = particle->GetParticleName();
    fDosePerParticle[name] += energyLoss;

    return true;

}


const std::map<G4String, G4double>&
RADSensitiveDetector::GetDoseMap() const
{
    return fDosePerParticle;
}
