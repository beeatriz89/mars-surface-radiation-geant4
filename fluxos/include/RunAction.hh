/// \file B1/include/RunAction.hh
/// \brief Definition of the B1::RunAction class

#ifndef B1RunAction_h
#define B1RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

#include <vector>
#include <string>

#include <map>
#include "G4LogicalVolume.hh"
#include "G4Threading.hh"

class G4Run;
class RADSensitiveDetector;

namespace B1
{

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    ~RunAction() override = default;

    // Override GenerateRun to return our custom Run class
    G4Run* GenerateRun() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    //void AddEdep (G4double edep);
    //void AddEquivalentDose(G4double dose);
    //void AddEdepByParticle(G4String particle, G4double edep);
    //void AddFlux(const G4String& direction,
      //           const G4String& particle,
        //         G4int count,
          //       G4double energy);

    // NOVO: Adicionar fluxo com energia para histograma
    //void AddFluxSpectrum(G4String direction, G4String particle, G4double energy);

    //void AddFlux(const G4String& direction,
      //           const G4String& particle,
        //         G4double energy);

    //void Merge(const RunAction* other);


  private:

    //G4Accumulable<G4double> fEdep = 0.;
    //G4Accumulable<G4double> fEdep2 = 0.;
    //G4double fEquivalentDose = 0.;
    //G4double fEquivalentDose2 = 0.;

    //std::map<G4String, std::map<G4String, G4int>> fFluxCount;
    //std::map<G4String, std::map<G4String, G4double>> fFluxEnergy;
    //std::map<G4String, G4double> fEdepByParticle;

    //std::map<G4String, std::map<G4String, std::vector<G4double>>> fFluxSpectra;

    //std::mutex fSpectraMutex;  // Para thread-safety

    G4LogicalVolume* fScoringVolume = nullptr;

    //void PrintFluxTable(G4String direction, G4int nEvents);
    void WriteFluxSpectra(const G4Run* run);  // NOVO: Escrever ficheiros
    //G4int GetEnergyBin(G4double energy);  // NOVO: Binning de energia
};

}

#endif

