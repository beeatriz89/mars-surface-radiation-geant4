#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "G4EmCalculator.hh"
#include "globals.hh"

class DetectorConstruction;
class EventAction;
class G4Step;

// Acumula, para cada passo dentro da placa:
//   - dose (edep / massa da placa)
//   - dose ponderada por Q(L), onde L é o LET (poder de paragem não
//     restrito) da partícula naquele passo, obtido das tabelas de
//     física via G4EmCalculator na energia local (mais robusto que
//     edep/stepLength, que é sensível ao tamanho do passo/MSC)
//   - dose ponderada por L (para o LET médio ponderado à dose no fim)
//
// Os totais de cada passo são entregues ao EventAction (que agrega por
// evento) em vez de irem diretamente para o RunAction — isto permite
// calcular a incerteza estatística (cada evento = uma amostra).
//
// Q(L) segue ICRP 60 / Anexo B do ICRP 103, referido no ICRP 123 como
// a abordagem recomendada para radiação espacial:
//   Q(L) = 1                    , L < 10 keV/um
//   Q(L) = 0.32 L - 2.2         , 10 <= L <= 100 keV/um
//   Q(L) = 300 / sqrt(L)        , L > 100 keV/um
//
// Partículas neutras (fotões, neutrões) que depositem energia localmente
// (abaixo do corte de produção, sem gerar uma partícula secundária
// rastreada) são tratadas como baixo LET (Q=1). Para fotões isto é uma
// boa aproximação (a energia acaba por vir de eletrões de baixo LET não
// rastreados). Para neutrões é menos frequente — a maior parte da dose
// de neutrões vem de recuos (protão, alfa, núcleo) que SÃO rastreados
// como partículas carregadas com o seu próprio LET/Q calculado normalmente;
// só o deposito direto do próprio passo do neutrão (raro) cai neste caso.
class SteppingAction : public G4UserSteppingAction
{
  public:
    SteppingAction(const DetectorConstruction* detector, EventAction* eventAction);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

  private:
    static G4double QualityFactor(G4double LET_keV_per_um);

    const DetectorConstruction* fDetector = nullptr;
    EventAction* fEventAction = nullptr;
    G4EmCalculator fEmCalculator; // uma instância por thread (SteppingAction é per-thread)
};

#endif

