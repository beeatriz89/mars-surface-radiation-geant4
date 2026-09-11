#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "BeamConfig.hh"

#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"

#include <fstream>
#include <cmath>

RunAction::RunAction(const DetectorConstruction* detector)
  : fDetector(detector)
{
  G4AccumulableManager* accManager = G4AccumulableManager::Instance();
  accManager->Register(fDose);
  accManager->Register(fDoseSq);
  accManager->Register(fWeightedDose);
  accManager->Register(fWeightedDoseSq);
  accManager->Register(fLETDose);

  fMessenger = new G4GenericMessenger(this, "/output/", "Configuracao do ficheiro de saida");
  fMessenger->DeclareMethod("csvFile", &RunAction::SetCsvFileName)
      .SetGuidance("Nome do ficheiro CSV onde anexar os resultados "
                    "(default: conversion_factors.csv). Usa nomes diferentes "
                    "para piloto vs producao.");
  fMessenger->DeclareMethod("primaryOnly", &RunAction::SetPrimaryOnly)
      .SetGuidance("true: so conta a particula primaria no scoring (ignora "
                   "secundarios/fragmentos). false (default): inclui tudo.");
}

RunAction::~RunAction()
{
  delete fMessenger;
}

void RunAction::SetCsvFileName(G4String name)
{
  BeamConfig::SetCsvFileName(name);
}

void RunAction::SetPrimaryOnly(G4bool enabled)
{
  BeamConfig::SetPrimaryOnlyMode(enabled);
}

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AccumulableManager::Instance()->Reset();
}

void RunAction::AddEventSample(G4double doseEvt, G4double weightedDoseEvt, G4double letDoseEvt)
{
  fDose         += doseEvt;
  fDoseSq       += doseEvt * doseEvt;
  fWeightedDose += weightedDoseEvt;
  fWeightedDoseSq += weightedDoseEvt * weightedDoseEvt;
  fLETDose      += letDoseEvt;
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  G4int nEvents = run->GetNumberOfEvent();
  if (nEvents == 0) return;

  // Junta as contribuições de todas as worker threads no master.
  // Em modo sequencial (sem MT) isto é uma no-op inofensiva.
  G4AccumulableManager::Instance()->Merge();

  // Só o master tem os totais corretos (já com o merge de todas as
  // threads); as workers individuais teriam apenas o seu run local.
  if (!IsMaster()) return;

  // --- Médias e incerteza estatística por-primário ---
  // Cada evento é uma amostra independente x_i (dose depositada por esse
  // primário, incluindo secundários). Erro padrão da média = sigma/sqrt(N).
  G4double N = static_cast<G4double>(nEvents);

  G4double meanDose   = fDose.GetValue() / N;
  G4double meanDoseSq = fDoseSq.GetValue() / N;
  G4double varDose    = std::max(0.0, meanDoseSq - meanDose * meanDose);
  G4double semDose    = std::sqrt(varDose / N);
  G4double relUncDose = (meanDose > 0.0) ? semDose / meanDose : 0.0;

  G4double meanH   = fWeightedDose.GetValue() / N;
  G4double meanHSq = fWeightedDoseSq.GetValue() / N;
  G4double varH    = std::max(0.0, meanHSq - meanH * meanH);
  G4double semH    = std::sqrt(varH / N);
  G4double relUncH = (meanH > 0.0) ? semH / meanH : 0.0;

  // LET e Q médios, ponderados à dose (razão de somas: unidades internas cancelam)
  G4double letDoseAvg = (fDose.GetValue() > 0.0) ? fLETDose.GetValue() / fDose.GetValue() : 0.0;
  G4double qDoseAvg   = (fDose.GetValue() > 0.0) ? fWeightedDose.GetValue() / fDose.GetValue() : 0.0;

  // --- Conversão para unidades físicas ---
  G4double totalDoseGy = fDose.GetValue() / gray;
  G4double totalHSv    = fWeightedDose.GetValue() / gray; // "gray" == J/kg == unidade base de Sv também

  G4double halfX = fDetector->GetSlabHalfX();
  G4double halfY = fDetector->GetSlabHalfY();
  G4double areaCm2 = (2. * halfX / cm) * (2. * halfY / cm);

  G4double fluence = N / areaCm2; // partículas / cm^2

  G4double convFactorDose  = totalDoseGy / fluence; // Gy . cm^2
  G4double convFactorEquiv = totalHSv / fluence;    // Sv . cm^2

  G4String particleName = BeamConfig::GetParticleName();
  G4double energyMeV    = BeamConfig::GetEnergyMeV();

  G4cout << "\n=================== RESULTADOS DO RUN ===================\n"
         << "  Particula:              " << particleName << "\n"
         << "  Energia cinetica:       " << energyMeV << " MeV\n"
         << "  N. de eventos:          " << nEvents << "\n"
         << "  Area da fonte:          " << areaCm2 << " cm^2\n"
         << "  Fluencia incidente:     " << fluence << " particulas/cm^2\n"
         << "  LET medio (dose-avg):   " << letDoseAvg << " keV/um\n"
         << "  Q medio (dose-avg):     " << qDoseAvg << "\n"
         << "  Dose absorvida total:   " << totalDoseGy << " Gy  (+/- "
         << relUncDose * 100.0 << " %)\n"
         << "  Dose equivalente (H):   " << totalHSv << " Sv  (+/- "
         << relUncH * 100.0 << " %)\n"
         << "  -----------------------------------------------------------\n"
         << "  Fator conversao D/Phi:  " << convFactorDose  << " Gy.cm^2\n"
         << "  Fator conversao H/Phi:  " << convFactorEquiv << " Sv.cm^2\n"
         << "===========================================================\n"
         << G4endl;

  // --- Escrever/anexar uma linha no CSV ---
  G4String csvFileName = BeamConfig::GetCsvFileName();

  bool fileIsEmpty = true;
  { std::ifstream check(csvFileName); fileIsEmpty = (check.peek() == std::ifstream::traits_type::eof()); }

  std::ofstream csv(csvFileName, std::ios::app);
  if (fileIsEmpty) {
    csv << "Particle,Energy_MeV,N_events,Area_cm2,Fluence_per_cm2,"
        << "LET_doseAvg_keV_per_um,Q_doseAvg,"
        << "Dose_Gy,Dose_relUncert_pct,DosePerFluence_Gy_cm2,"
        << "H_Sv,H_relUncert_pct,HPerFluence_Sv_cm2\n";
  }
  csv << particleName << "," << energyMeV << "," << nEvents << "," << areaCm2 << ","
      << fluence << "," << letDoseAvg << "," << qDoseAvg << ","
      << totalDoseGy << "," << relUncDose * 100.0 << "," << convFactorDose << ","
      << totalHSv << "," << relUncH * 100.0 << "," << convFactorEquiv << "\n";
}
