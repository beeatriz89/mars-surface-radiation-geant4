#ifndef BeamConfig_h
#define BeamConfig_h 1

#include "globals.hh"

// Pequeno estado partilhado entre threads, usado apenas para fins de
// reporte (nome da partícula e energia). Em MT, o master não tem uma
// PrimaryGeneratorAction (BuildForMaster só cria o RunAction), pelo que
// o RunAction do master não consegue perguntar à gun qual a partícula
// configurada. Cada worker escreve aqui o mesmo valor (definido pela
// mesma macro), por isso a escrita concorrente é inofensiva; protegemos
// na mesma com um mutex por higiene.
namespace BeamConfig
{
  void SetParticleName(const G4String& name);
  G4String GetParticleName();

  void SetEnergyMeV(G4double energyMeV);
  G4double GetEnergyMeV();

  // Nome do ficheiro CSV de saída (default "conversion_factors.csv").
  // Configurável por macro com /output/csvFile <nome>, para poderes
  // separar corridas piloto (para estimar N) de corridas de produção.
  void SetCsvFileName(const G4String& name);
  G4String GetCsvFileName();

  // Modo "só partícula primária" no scoring (default: false, i.e.
  // inclui todos os secundários — comportamento original). Configurável
  // por macro com /scoring/primaryOnly true|false. Ver SteppingAction.cc.
  void SetPrimaryOnlyMode(G4bool enabled);
  G4bool GetPrimaryOnlyMode();

}

#endif
