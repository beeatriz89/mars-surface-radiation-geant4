#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "BeamConfig.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "G4UIcmdWith3Vector.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(const DetectorConstruction* detector)
  : fDetector(detector)
{
  fParticleGun = new G4ParticleGun(1);

  // Defaults: fotões de 1 MeV
  G4ParticleTable* table = G4ParticleTable::GetParticleTable();
  fParticleGun->SetParticleDefinition(table->FindParticle("gamma"));
  fParticleGun->SetParticleEnergy(1.0 * MeV);
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., -1.));
  BeamConfig::SetParticleName("gamma");
  BeamConfig::SetEnergyMeV(1.0);

  // Área da fonte = área da face de entrada da placa (garante que
  // todo o feixe entra na placa e a fluência fica bem definida)
  G4double halfX = fDetector->GetSlabHalfX();
  G4double halfY = fDetector->GetSlabHalfY();
  fAreaCm2 = (2. * halfX / cm) * (2. * halfY / cm);

  // Fonte acima da face de entrada da placa (+z). Como o feixe é
  // paralelo/mono-direcional e o World é vácuo, esta distância não
  // tem qualquer efeito físico — serve só para clareza geométrica.
  fSourceZ = fDetector->GetSlabFrontZ() + 28.0 * mm + 10.0 * mm;

  // --- Comandos de macro para configurar a fonte ---
  // NOTA: usamos "/source/" (nao "/gun/") de proposito. O G4ParticleGun
  // ja cria automaticamente os seus proprios comandos em "/gun/particle",
  // "/gun/energy", "/gun/ion", etc (via G4ParticleGunMessenger interno).
  // Registar os nossos com os MESMOS nomes em "/gun/" colide com esses
  // comandos nativos - o Geant4 acaba por executar a versao nativa em
  // vez da nossa, sem avisar claramente (so um warning tardio tipo
  // "Set /gun/particle to ion before using /gun/ion command"). O resultado
  // pratico e que a particula/energia reais nunca mudam como esperado.
  fMessenger = new G4GenericMessenger(this, "/source/", "Configuração da fonte plana AP");

  fMessenger->DeclareMethod("particle", &PrimaryGeneratorAction::SetParticleByName)
      .SetGuidance("Nome da partícula (gamma, e-, e+, proton, pi+, pi-, mu+, mu-, neutron, ...)");

  fMessenger->DeclareMethodWithUnit("energy", "MeV", &PrimaryGeneratorAction::SetEnergy)
      .SetGuidance("Energia cinética da partícula (não usar para iões; usar /source/ion)");

  // /source/ion Z A E_por_nucleao(MeV) — G4UIcmdWith3Vector nativo, não
  // G4GenericMessenger (que não suporta métodos de 3 argumentos aqui)
  fIonCmd = new G4UIcmdWith3Vector("/source/ion", this);
  fIonCmd->SetGuidance("Define um ião: Z A E(MeV/nucleao). Ex: /source/ion 2 4 5.0 -> alfa a 5 MeV/u");
  fIonCmd->SetParameterName("Z", "A", "EperNucleonMeV", false);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fMessenger;
  delete fIonCmd;
}

void PrimaryGeneratorAction::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fIonCmd) {
    G4ThreeVector v = fIonCmd->GetNew3VectorValue(newValue); // (Z, A, E_por_nucleao)
    SetIon(static_cast<G4int>(v.x()), static_cast<G4int>(v.y()), v.z());
  }
}

void PrimaryGeneratorAction::SetParticleByName(G4String name)
{
  G4ParticleTable* table = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* p = table->FindParticle(name);
  if (p) {
    fParticleGun->SetParticleDefinition(p);
    BeamConfig::SetParticleName(name);
  }
}

void PrimaryGeneratorAction::SetEnergy(G4double energy)
{
  fParticleGun->SetParticleEnergy(energy);
  BeamConfig::SetEnergyMeV(energy / MeV);
}

void PrimaryGeneratorAction::SetIon(G4int Z, G4int A, G4double energyPerNucleonMeV)
{
  G4double totalEnergy = energyPerNucleonMeV * MeV * A;

  G4ParticleDefinition* ion = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
  fParticleGun->SetParticleDefinition(ion);
  fParticleGun->SetParticleEnergy(totalEnergy);

  BeamConfig::SetParticleName(ion->GetParticleName());
  BeamConfig::SetEnergyMeV(totalEnergy / MeV);
}

const G4ParticleDefinition* PrimaryGeneratorAction::GetParticleDefinition() const
{
  return fParticleGun->GetParticleDefinition();
}

G4double PrimaryGeneratorAction::GetParticleEnergy() const
{
  return fParticleGun->GetParticleEnergy();
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  G4double halfX = fDetector->GetSlabHalfX();
  G4double halfY = fDetector->GetSlabHalfY();

  G4double x = (2.0 * G4UniformRand() - 1.0) * halfX;
  G4double y = (2.0 * G4UniformRand() - 1.0) * halfY;

  fParticleGun->SetParticlePosition(G4ThreeVector(x, y, fSourceZ));
  fParticleGun->GeneratePrimaryVertex(event);
}
