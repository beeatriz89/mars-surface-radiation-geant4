#ifndef RADSensitiveDetector_h
#define RADSensitiveDetector_h 1

#include "G4VSensitiveDetector.hh"
#include <map>

class RADSensitiveDetector : public G4VSensitiveDetector
{
public:
    RADSensitiveDetector(const G4String& name);
    ~RADSensitiveDetector() override = default;

    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

    const std::map<G4String, G4double>& GetDoseMap() const;

private:
    std::map<G4String, G4double> fDosePerParticle;
};

#endif
