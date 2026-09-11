#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"
#include "G4Exception.hh"

#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

namespace B1
{

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction(),
      fParticleGun(new G4ParticleGun(1)),
      fMessenger(new G4GenericMessenger(
          this,
          "/generator/",
          "Primary generator control")),

      fParticle(nullptr),
      fParticleName(""),
      fDirection("downward"),

      fSourceRadius(1.5 * m),
      fDownwardSourceZ(16 * m),
      fUpwardSourceZ(11.05 * m)
{
    fMessenger->DeclareMethod(
        "setParticle",
        &PrimaryGeneratorAction::SetParticle,
        "Set Geant4 particle name");

    fMessenger->DeclareMethod(
        "setIon",
        &PrimaryGeneratorAction::SetIon,
        "Set ion using Z and A");

    fMessenger->DeclareMethod(
        "setSpectrum",
        &PrimaryGeneratorAction::SetSpectrum,
        "Load energy spectrum file");

    fMessenger->DeclareMethod(
        "setDirection",
        &PrimaryGeneratorAction::SetDirection,
        "Set source direction: downward or upward");

    fMessenger->DeclarePropertyWithUnit(
        "sourceRadius", "m",
        fSourceRadius,
        "Radius of circular planar source [m]");
 
    fMessenger->DeclarePropertyWithUnit(
        "downwardSourceZ", "m",
        fDownwardSourceZ,
        "Z position of downward source plane [m]");
 
    fMessenger->DeclarePropertyWithUnit(
        "upwardSourceZ", "m",
        fUpwardSourceZ,
        "Z position of upward source plane [m]");

}

// ============================================================================
// Destructor
// ============================================================================

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fMessenger;   // nullptr-safe; only master has non-null fMessenger
    delete fParticleGun;
}

// ============================================================================
// SetParticle
// ============================================================================

void PrimaryGeneratorAction::SetParticle(const G4String& name)
{
    auto* particle =
        G4ParticleTable::GetParticleTable()->FindParticle(name);

    if (!particle) {

        G4Exception(
            "PrimaryGeneratorAction::SetParticle",
            "UnknownParticle",
            FatalException,
            ("Unknown particle: " + name).c_str()
        );
    }

    fParticle = particle;
    fParticleName = name;

    G4cout
        << "Primary particle set: "
        << fParticleName
        << G4endl;
}

// ============================================================================
// SetIon
// ============================================================================

void PrimaryGeneratorAction::SetIon(G4int Z, G4int A)
{
    auto* ion =
        G4IonTable::GetIonTable()->GetIon(Z, A, 0.);

    if (!ion) {

        G4Exception(
            "PrimaryGeneratorAction::SetIon",
            "IonNotFound",
            FatalException,
            "Could not create requested ion."
        );
    }

    fParticle = ion;
    fParticleName = ion->GetParticleName();

    G4cout
        << "Primary ion set: Z="
        << Z
        << " A="
        << A
        << " ("
        << fParticleName
        << ")"
        << G4endl;
}

// ============================================================================
// SetSpectrum
// ============================================================================

void PrimaryGeneratorAction::SetSpectrum(const G4String& filename)
{
    LoadSpectrum(filename);
}

// ============================================================================
// LoadSpectrum
// ============================================================================

void PrimaryGeneratorAction::LoadSpectrum(const G4String& filename)
{
    fEnergyBins.clear();
    fEnergyProbs.clear();

    std::ifstream file(filename);

    if (!file.is_open()) {

        G4Exception(
            "PrimaryGeneratorAction::LoadSpectrum",
            "SpectrumFileError",
            FatalException,
            ("Could not open: " + filename).c_str()
        );
    }

    std::string line;

    G4int nLines = 0;
    G4int nComments = 0;
    G4int nInvalid = 0;
    G4int nNonPositive = 0;
    G4int nValid = 0;

    while (std::getline(file, line)) {

        nLines++;

        if (line.empty())
            continue;

        if (line[0] == '#') {
            nComments++;
            continue;
        }

        std::istringstream ss(line);

        G4double energy;
        G4double probability;

        if (!(ss >> energy >> probability)) {
            nInvalid++;
            continue;
        }

        if (energy <= 0.0 || probability <= 0.0) {
            nNonPositive++;
            continue;
        }

        fEnergyBins.push_back(energy);
        fEnergyProbs.push_back(probability);

        nValid++;
    }

    file.close();

    if (fEnergyBins.empty()) {

        G4Exception(
            "PrimaryGeneratorAction::LoadSpectrum",
            "EmptySpectrum",
            FatalException,
            ("No valid entries in: " + filename).c_str()
        );
    }

    // Normalize probabilities
    G4double sum = 0.0;

    for (G4double p : fEnergyProbs)
        sum += p;

    for (G4double& p : fEnergyProbs)
        p /= sum;

    G4cout
        << "Spectrum loaded: "
        << filename
        << "  ("
        << nValid
        << " bins)"
        << G4endl;
}

// ============================================================================
// SetDirection
// ============================================================================

void PrimaryGeneratorAction::SetDirection(const G4String& dir)
{
    if (dir != "downward" && dir != "upward") {

        G4Exception(
            "PrimaryGeneratorAction::SetDirection",
            "InvalidDirection",
            FatalException,
            "Direction must be 'downward' or 'upward'."
        );
    }

    fDirection = dir;

    G4cout
        << "Source direction set: "
        << fDirection
        << G4endl;
}

// ============================================================================
// SampleEnergy
// ============================================================================

G4double PrimaryGeneratorAction::SampleEnergy()
{
    G4double r = G4UniformRand();

    G4double cumulative = 0.0;

    for (std::size_t i = 0; i < fEnergyProbs.size(); ++i) {

        cumulative += fEnergyProbs[i];

        if (r <= cumulative)
            return fEnergyBins[i] * MeV;
    }

    return fEnergyBins.back() * MeV;
}

// ============================================================================
// SampleCosineDirection
// ============================================================================

G4ThreeVector PrimaryGeneratorAction::SampleCosineDirection()
{

    // Isotropic radiation incident on a planar surface:
    // p(cos(theta)) = 2 cos(theta)

    G4double cosTheta = std::sqrt(G4UniformRand());
    G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    G4double phi = CLHEP::twopi * G4UniformRand();

    G4double uz =
        (fDirection == "downward")
        ? -cosTheta
        : +cosTheta;

    return G4ThreeVector(
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        uz
    );

}

// ============================================================================
// SampleSourcePosition
// ============================================================================

G4ThreeVector PrimaryGeneratorAction::SampleSourcePosition()
{
    G4double r =
        fSourceRadius * std::sqrt(G4UniformRand());

    G4double phi =
        CLHEP::twopi * G4UniformRand();

    G4double z =
        (fDirection == "downward")
        ? fDownwardSourceZ
        : fUpwardSourceZ;

    return G4ThreeVector(
        r * std::cos(phi),
        r * std::sin(phi),
        z
    );
}

// ============================================================================
// GeneratePrimaries
// ============================================================================

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (!fParticle) {

        G4Exception(
            "PrimaryGeneratorAction::GeneratePrimaries",
            "ParticleNotSet",
            FatalException,
            "Particle not set — check /generator/setParticle."
        );
    }

    if (fEnergyBins.empty()) {

        G4Exception(
            "PrimaryGeneratorAction::GeneratePrimaries",
            "SpectrumNotSet",
            FatalException,
            "Spectrum not set — check /generator/setSpectrum."
        );
    }

    G4ThreeVector position = SampleSourcePosition();
    G4ThreeVector direction = SampleCosineDirection();

    G4double energy = SampleEnergy();
    static G4int debugSource = 0;

    if (debugSource < 20) {
        G4cout
        << "\n========== SOURCE DEBUG ==========\n"
        << "Position  = "
        << position / m << " m\n"
        << "Direction = "
        << direction << "\n"
        << "Energy    = "
        << SampleEnergy() / MeV << " MeV\n"
        << "==================================\n"
        << G4endl;

        debugSource++;
}

    fParticleGun->SetParticleDefinition(fParticle);

    fParticleGun->SetParticlePosition(
        SampleSourcePosition()
    );

    fParticleGun->SetParticleMomentumDirection(
        SampleCosineDirection()
    );

    G4double energyPerNucleon = SampleEnergy();

    G4int A = fParticle->GetBaryonNumber();

    G4double totalEnergy =
        (A > 1)
        ? A * energyPerNucleon
        : energyPerNucleon;

    fParticleGun->SetParticleEnergy(totalEnergy);

    fParticleGun->GeneratePrimaryVertex(event);
}

}
