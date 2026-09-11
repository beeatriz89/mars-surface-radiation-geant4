/// \file B1/src/SteppingAction.cc
/// \brief Implementation of the B1::SteppingAction class

#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "Run.hh"

#include "RunAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

#include "G4ParticleDefinition.hh"

#include "G4Gamma.hh"
#include "G4Electron.hh"
#include "G4Neutron.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

#include <cmath>

namespace B1

{

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fScoringVolume(nullptr),
  fScoringZ(11*m),
  fSurfaceTolerance(0.1*mm)
{}

SteppingAction::~SteppingAction()
{}

G4bool SteppingAction::IsCrossingSurface(const G4Step* step, G4double zSurface, G4bool& isDownward)
{
  G4ThreeVector prePos = step->GetPreStepPoint()->GetPosition();
  G4ThreeVector postPos = step->GetPostStepPoint()->GetPosition();

  G4double preZ = prePos.z();
  G4double postZ = postPos.z();

  // Verifica se cruzou a superfície
  if ((preZ > zSurface && postZ <= zSurface) ||
      (preZ < zSurface && postZ >= zSurface)) {

    // Determina direção (true = para baixo, false = para cima)
    isDownward = (postZ < preZ);
    return true;
  }

  return false;
}

///G4double SteppingAction::GetZenithAngle(const G4ThreeVector& direction)
///{
  // Ângulo zenital em relação ao eixo +Z
  // θ = arccos(dz), onde dz é a componente Z do vetor direção unitário
  ///G4double cosTheta = - direction.z();
  ///G4double theta = std::acos(cosTheta);
  ///return theta;
///}

G4String SteppingAction::GetParticleCategory(const G4ParticleDefinition* pdef)
{
  switch (pdef->GetPDGEncoding()) {
    case 2212:       return "H";
    case 2112:       return "n";
    case 22:         return "gamma";
    case 11:         return "e-";
    case -11:        return "e+";
    case 13:         return "mu-";
    case -13:        return "mu+";
    case 211:        return "pi+";
    case -211:       return "pi-";
    case 1000010020: return "2H";
    case 1000010030: return "3H";
    case 1000020030: return "3He";
    case 1000020040: return "4He";
    default: break;
  }

  if (pdef->GetParticleType() == "nucleus") {
    const G4int Z = pdef->GetAtomicNumber();
    if (Z >= 3 && Z <= 28) return "ion_Z" + G4String(std::to_string(Z));
  }
  return "other";
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{

  G4Track* track = step->GetTrack();
  if (track->GetParticleDefinition() == G4Neutron::Definition()
      && track->GetKineticEnergy() < 0.4*MeV) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  // e- e gamma: matar assim que caem abaixo do limiar de scoring.
  // NOTA: e+ fica deliberadamente de fora - aniquilação em voo pode
  // produzir gamas acima do limiar mesmo com KE do e+ ~0.
  if ((track->GetParticleDefinition() == G4Gamma::Definition())
      && track->GetKineticEnergy() < 0.4 * MeV) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  // watchdog against pathological tracks (stuck at boundaries, etc.)
  if (track->GetCurrentStepNumber() > 2000000) {
    G4cout << "[WATCHDOG] killing " << track->GetDefinition()->GetParticleName()
           << " E=" << track->GetKineticEnergy()/MeV << " MeV after 2e6 steps" << G4endl;
    track->SetTrackStatus(fStopAndKill);
    return;
  }


  if (!fScoringVolume) {
    const auto detConstruction = static_cast<const DetectorConstruction*>(
      G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    fScoringVolume = detConstruction->GetScoringVolume();

  }

  G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()
                            ->GetVolume()->GetLogicalVolume();

  if (volume == fScoringVolume) {

    G4double edepStep = step->GetTotalEnergyDeposit();

    if (edepStep > 0) {
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4double kineticEnergy = track->GetKineticEnergy();

      // F6 Tally: Dose absorvida
    fEventAction->AddEdep(edepStep);

    }
  }

  G4bool isDownward = false;
  G4bool isCrossing = IsCrossingSurface(step, fScoringZ, isDownward);
  if (!isCrossing) return;

  G4StepPoint* preStep = step->GetPreStepPoint();
  G4ThreeVector direction = preStep->GetMomentumDirection();
  G4double kineticEnergy = preStep->GetKineticEnergy();

  G4String category = GetParticleCategory(track->GetDefinition());
  if (category == "other") return;  // Ignora partículas não listadas

  G4double cosTheta_z = std::abs(direction.z());
  if (cosTheta_z < 0.001) return;
  G4double weight = 1.0 / cosTheta_z;

  G4double zenithDeg = std::acos(std::min(1.0, std::max(-1.0, -direction.z())))
                       * 180.0 / CLHEP::pi;

  // Get thread-local Run for spectrum accumulation
  auto* run = static_cast<Run*>(
    G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  // === Score with weight ===
  fEventAction->AddFlux("4pi", category, kineticEnergy);
  if (run) run->AddFluxSpectrum("4pi", category, kineticEnergy, weight);

  if (isDownward) {
    fEventAction->AddFlux("downward", category, kineticEnergy);
    if (run) run->AddFluxSpectrum("downward", category, kineticEnergy, weight);
    if (zenithDeg <= 30.0) {
      fEventAction->AddFlux("RAD_cone", category, kineticEnergy);
      if (run) run->AddFluxSpectrum("RAD_cone", category, kineticEnergy, weight);
    }
  } else {
    fEventAction->AddFlux("upward", category, kineticEnergy);
    if (run) run->AddFluxSpectrum("upward", category, kineticEnergy, weight);
  }


}

}
