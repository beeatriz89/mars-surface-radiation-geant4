#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UImessenger.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4Event;
class DetectorConstruction;
class G4GenericMessenger;
class G4ParticleDefinition;
class G4UIcmdWith3Vector;
class G4UIcommand;

// Fonte plana, paralela e mono-direcional (geometria AP):
// - posição (x,y) sorteada uniformemente na área da face de
//   entrada da placa; z fixo, um pouco acima da placa
// - direção fixa (0,0,-1): feixe perpendicular à placa
// Configurável por macro:
//   /source/particle <nome>          (ex: gamma, e-, e+, proton, pi+, mu-)
//   /source/ion <Z> <A> <E_por_nucleao_MeV>   (ex: alfa a 10 MeV/u = /source/ion 2 4 10)
//   /source/energy <valor> <unidade> (energia cinética, para não-iões)
//
// NOTA: /source/particle e /source/energy usam G4GenericMessenger (só
// aceita métodos de 1 argumento). /source/ion precisa de 3 valores, e
// o G4GenericMessenger::DeclareMethod não suporta métodos multi-argumento
// no Geant4 11.x (dá erro de compilação: "cannot convert ... to const
// G4AnyMethod&"). Por isso /source/ion é implementado à parte, com
// G4UIcmdWith3Vector (a classe nativa do Geant4 para "3 números
// separados por espaço"), herdando G4UImessenger diretamente.
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction, public G4UImessenger
{
  public:
    explicit PrimaryGeneratorAction(const DetectorConstruction* detector);
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Callback do G4UImessenger para o comando /source/ion
    void SetNewValue(G4UIcommand* command, G4String newValue) override;

    // Área da fonte usada nesta geração, em cm^2 (número simples,
    // já sem "unidade" G4 embutida), para normalização da fluência
    G4double GetSourceAreaCm2() const { return fAreaCm2; }

    // Acesso à partícula/energia atualmente configuradas na gun
    // (disponível para uso futuro; o RunAction usa o BeamConfig,
    // pois no master não existe instância desta classe em modo MT)
    const G4ParticleDefinition* GetParticleDefinition() const;
    G4double GetParticleEnergy() const; // em unidades internas G4 (MeV nativo)

  private:
    void SetParticleByName(G4String name);
    void SetEnergy(G4double energy);
    // Z, A, energia POR NUCLEÃO em MeV (número simples, sem unidade G4
    // embutida - multiplicamos por MeV manualmente no .cc)
    void SetIon(G4int Z, G4int A, G4double energyPerNucleonMeV);

    G4ParticleGun* fParticleGun = nullptr;
    const DetectorConstruction* fDetector = nullptr;
    G4GenericMessenger* fMessenger = nullptr;   // /source/particle, /source/energy
    G4UIcmdWith3Vector* fIonCmd = nullptr;      // /source/ion Z A E

    G4double fAreaCm2 = 0.0;
    G4double fSourceZ = 0.0;
};

#endif
