/// \file Run.hh
/// \brief Custom G4Run subclass for thread-safe data merging in MT mode

#ifndef B1Run_h
#define B1Run_h 1

#include "G4Run.hh"
#include "globals.hh"
#include <map>
#include <vector>
#include <utility>
#include <mutex>
#include <array>

namespace B1
{

class Run : public G4Run
{
  public:
    Run();
    ~Run() override = default;

    // Binning definido num único sítio (usado no fill e na escrita)
    static constexpr G4int    kNBins   = 85;
    static constexpr G4double kEminMeV = 1.04;
    static constexpr G4double kEmaxMeV = 9.26e5;

    struct Histo {
      std::array<G4double, kNBins> weight{};  // soma de 1/cos(theta)
      std::array<G4double, kNBins> weightSq{};     // NOVO: soma de (1/cos(theta))^2
      std::array<G4long,   kNBins> count{};   // contagem crua (para incerteza)
      G4long nUnder = 0;
      G4long nOver  = 0;
      G4long nTotal = 0;
    };

    void Merge(const G4Run* aRun) override;

    // --- Data accumulation methods (called from worker threads) ---
    void AddEdep(G4double edep);
    void AddEdep2(G4double edep2);
    void AddEquivalentDose(G4double dose);
    void AddEquivalentDose2(G4double dose2);
    void AddEdepByParticle(const G4String& particle, G4double edep);
    void AddFluxCount(const G4String& direction, const G4String& particle, G4int count);
    void AddFluxEnergy(const G4String& direction, const G4String& particle, G4double energy);
    void AddFluxSpectrum(const G4String& direction, const G4String& particle, G4double energy, G4double weight);

    // --- Getters ---
    G4double GetEdep() const { return fEdep; }
    G4double GetEdep2() const { return fEdep2; }
    G4double GetEquivalentDose() const { return fEquivalentDose; }
    G4double GetEquivalentDose2() const { return fEquivalentDose2; }

    const std::map<G4String, G4double>& GetEdepByParticle() const { return fEdepByParticle; }
    const std::map<G4String, std::map<G4String, G4int>>& GetFluxCount() const { return fFluxCount; }
    const std::map<G4String, std::map<G4String, G4double>>& GetFluxEnergy() const { return fFluxEnergy; }
    const std::map<G4String, std::map<G4String, Histo>>& GetFluxSpectra() const
    { return fFluxSpectra; }
    static const std::vector<G4double>& EnergyBinEdges();

  private:
    static G4int FindBin(G4double energy);

    G4double fEdep = 0.;
    G4double fEdep2 = 0.;
    G4double fEquivalentDose = 0.;
    G4double fEquivalentDose2 = 0.;

    std::vector<G4double> fEnergyBinEdges;
    std::map<G4String, G4double> fEdepByParticle;
    std::map<G4String, std::map<G4String, G4int>> fFluxCount;
    std::map<G4String, std::map<G4String, G4double>> fFluxEnergy;
    std::map<G4String, std::map<G4String, Histo>> fFluxSpectra;
};

}

#endif
