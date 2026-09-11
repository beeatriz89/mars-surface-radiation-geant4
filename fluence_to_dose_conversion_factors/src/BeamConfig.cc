#include "BeamConfig.hh"
#include "G4Threading.hh"
#include "G4AutoLock.hh"

namespace {
  G4Mutex beamConfigMutex = G4MUTEX_INITIALIZER;
  G4String gParticleName = "gamma";
  G4double gEnergyMeV = 1.0;
  G4String gCsvFileName = "conversion_factors.csv";
  G4bool gPrimaryOnlyMode = false;
}

namespace BeamConfig
{
  void SetParticleName(const G4String& name)
  {
    G4AutoLock lock(&beamConfigMutex);
    gParticleName = name;
  }

  G4String GetParticleName()
  {
    G4AutoLock lock(&beamConfigMutex);
    return gParticleName;
  }

  void SetEnergyMeV(G4double energyMeV)
  {
    G4AutoLock lock(&beamConfigMutex);
    gEnergyMeV = energyMeV;
  }

  G4double GetEnergyMeV()
  {
    G4AutoLock lock(&beamConfigMutex);
    return gEnergyMeV;
  }

  void SetCsvFileName(const G4String& name)
  {
    G4AutoLock lock(&beamConfigMutex);
    gCsvFileName = name;
  }

  G4String GetCsvFileName()
  {
    G4AutoLock lock(&beamConfigMutex);
    return gCsvFileName;
  }

  void SetPrimaryOnlyMode(G4bool enabled)
  {
    G4AutoLock lock(&beamConfigMutex);
    gPrimaryOnlyMode = enabled;
  }

  G4bool GetPrimaryOnlyMode()
  {
    G4AutoLock lock(&beamConfigMutex);
    return gPrimaryOnlyMode;
  }
}
