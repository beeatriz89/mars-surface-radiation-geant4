#include "StackingAction.hh"
#include "Run.hh"

#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"

namespace B1
{

namespace {
  // Energia mais baixa efetivamente pontuada no espectro
  const G4double kScoreEmin   = 0.4 * MeV;   // 1.04 MeV
  // Limiar separado, mais conservador, para neutrões (gamas de captura)
  const G4double kNeutronEmin = 0.4 * MeV;
}

G4ClassificationOfNewTrack
StackingAction::ClassifyNewTrack(const G4Track* track)
{
  // O primário nunca é morto
  if (track->GetParentID() == 0) return fUrgent;

  const G4ParticleDefinition* pdef = track->GetDefinition();
  const G4int    pdg  = pdef->GetPDGEncoding();
  const G4double eKin = track->GetKineticEnergy();

  if (pdg == 2112) return (eKin < kNeutronEmin) ? fKill : fUrgent;

  if (pdg == 22)
    return (eKin < kScoreEmin) ? fKill : fUrgent;

  if (pdef->GetPDGCharge() != 0.0 && eKin < kScoreEmin) return fKill;

  return fUrgent;
}

}
