#include "SteppingAction.hh"
#include "DetectorConstruction.hh"
#include "EventAction.hh"
#include "BeamConfig.hh"

#include "G4NistManager.hh"
#include "G4EmCalculator.hh"
#include "G4Proton.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

SteppingAction::SteppingAction(const DetectorConstruction* detector, EventAction* eventAction)
  : fDetector(detector), fEventAction(eventAction)
{}

G4double SteppingAction::QualityFactor(G4double LET_keV_per_um)
{
  if (LET_keV_per_um < 10.0)  return 1.0;
  if (LET_keV_per_um <= 100.0) return 0.32 * LET_keV_per_um - 2.2;
  return 300.0 / std::sqrt(LET_keV_per_um);
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  // So nos interessa energia depositada dentro da placa de tecido
  G4LogicalVolume* volume =
      step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
  if (volume != fDetector->GetSlabLogicalVolume()) return;

  const G4Track* track = step->GetTrack();

  // *** MODO "SO PARTICULA PRIMARIA" (TrackID == 1, estrito) ***
  // So faz sentido para primarios COM CARGA (protao, ioes, leptoes,
  // mesoes) - esses depositam dose continuamente ao longo do proprio
  // track, por isso "so o primario" e uma simplificacao valida que
  // exclui fragmentos indesejados de reacoes nucleares.
  //
  // Para primarios NEUTROS (gamma, neutron), isto nao faz sentido: eles
  // nao depositam dose diretamente (nao ionizam) - TODA a dose vem de
  // secundarios (eletroes Compton/fotoeletricos para o gamma; recuos de
  // protao/alfa/nucleo para o neutrao). Filtrar TrackID==1 para estes
  // exclui a fisica toda, dando D=H=0 (o bug que apanhamos). Por isso,
  // para gamma/neutron, o filtro fica sempre desligado, independente do
  // modo configurado.
  const G4String& primaryName = BeamConfig::GetParticleName();
  G4bool primaryIsNeutral = (primaryName == "gamma" || primaryName == "neutron");

  if (BeamConfig::GetPrimaryOnlyMode() && !primaryIsNeutral && track->GetTrackID() != 1) {
    return;
  }


  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.0) return;

  G4double mass = fDetector->GetSlabMass();

  G4double doseStep = edep / mass;
  const G4ParticleDefinition* particle = track->GetDefinition();

  G4double LET_keV_per_um = 0.0;
  G4double kinE_pre = step->GetPreStepPoint()->GetKineticEnergy();
  G4double kinE_post = step->GetPostStepPoint()->GetKineticEnergy();
  G4double kinE_mid = (kinE_pre + kinE_post) / 2;

  if (particle->GetPDGCharge() != 0.0 && kinE_mid > 0.0){
    G4Material* water = fDetector->GetWaterMaterial();

    if (water != nullptr){
      G4double dEdx = fEmCalculator.ComputeElectronicDEDX( kinE_mid, particle, water);

      if (std::isfinite(dEdx) && dEdx > 0.0){
        LET_keV_per_um = dEdx / (keV / um);

        }
     }
  }


  // particulas neutras (fotoes, neutroes): LET_keV_per_um fica 0 -> Q(0) = 1 (baixo LET)

  G4double Q = QualityFactor(LET_keV_per_um);

  fEventAction->AddStepDose(doseStep);
  fEventAction->AddStepWeightedDose(Q * doseStep);
  fEventAction->AddStepLETDose(LET_keV_per_um * doseStep);

}
