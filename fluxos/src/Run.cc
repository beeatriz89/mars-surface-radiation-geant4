/// \file Run.cc
/// \brief Implementation of the B1::Run class with MT-safe Merge()

#include "Run.hh"
#include "G4SystemOfUnits.hh"
#include <algorithm>
#include <cmath>

namespace B1
{

Run::Run()
: G4Run()
{}

void Run::Merge(const G4Run* aRun)
{
  const auto* localRun = static_cast<const Run*>(aRun);

  // Scalars
  fEdep  += localRun->fEdep;
  fEdep2 += localRun->fEdep2;
  fEquivalentDose  += localRun->fEquivalentDose;
  fEquivalentDose2 += localRun->fEquivalentDose2;

  // Dose by particle
  for (const auto& pair : localRun->fEdepByParticle) {
    fEdepByParticle[pair.first] += pair.second;
  }

  // Flux counts
  for (const auto& dirPair : localRun->fFluxCount) {
    for (const auto& partPair : dirPair.second) {
      fFluxCount[dirPair.first][partPair.first] += partPair.second;
    }
  }

  // Flux energies
  for (const auto& dirPair : localRun->fFluxEnergy) {
    for (const auto& partPair : dirPair.second) {
      fFluxEnergy[dirPair.first][partPair.first] += partPair.second;
    }
  }

// Merge dos histogramas: soma bin a bin
  for (const auto& dirPair : localRun->fFluxSpectra) {
    for (const auto& partPair : dirPair.second) {
      Histo& dest = fFluxSpectra[dirPair.first][partPair.first];
      const Histo& src = partPair.second;
      for (G4int i = 0; i < kNBins; ++i) {
        dest.weight[i] += src.weight[i];
        dest.weightSq[i] += src.weightSq[i];
        dest.count[i]  += src.count[i];
      }
      dest.nUnder += src.nUnder;
      dest.nOver  += src.nOver;
      dest.nTotal += src.nTotal;
    }
  }

  // IMPORTANT: call base class Merge to handle event counting etc.
  G4Run::Merge(aRun);
}


// As arestas dos bins são calculadas exatamente como estavam em WriteFluxSpectra,
// para que o binning não mude em nada.
const std::vector<G4double>& Run::EnergyBinEdges()
{
  static const std::vector<G4double> edges = []() {
    std::vector<G4double> e;
    e.reserve(kNBins + 1);
    const G4double logEmin = std::log10(kEminMeV);
    const G4double logEmax = std::log10(kEmaxMeV);
    const G4double dLog = (logEmax - logEmin) / kNBins;
    for (G4int i = 0; i <= kNBins; ++i)
      e.push_back(std::pow(10.0, logEmin + i * dLog) * MeV);
    return e;
  }();
  return edges;
}

// Devolve -1 (underflow) ou kNBins (overflow) fora do domínio.
G4int Run::FindBin(G4double energy)
{
  const auto& edges = EnergyBinEdges();
  if (energy <  edges.front()) return -1;
  if (energy >= edges.back())  return kNBins;
  const auto it = std::upper_bound(edges.begin(), edges.end(), energy);
  return static_cast<G4int>(std::distance(edges.begin(), it)) - 1;
}


// Accumulation methods

void Run::AddEdep(G4double edep)
{
  fEdep += edep;
}

void Run::AddEdep2(G4double edep2)
{
  fEdep2 += edep2;
}

void Run::AddEquivalentDose(G4double dose)
{
  fEquivalentDose += dose;
}

void Run::AddEquivalentDose2(G4double dose2)
{
  fEquivalentDose2 += dose2;
}

void Run::AddEdepByParticle(const G4String& particle, G4double edep)
{
  fEdepByParticle[particle] += edep;
}

void Run::AddFluxCount(const G4String& direction, const G4String& particle, G4int count)
{
  fFluxCount[direction][particle] += count;
}

void Run::AddFluxEnergy(const G4String& direction, const G4String& particle, G4double energy)
{
  fFluxEnergy[direction][particle] += energy;
}

void Run::AddFluxSpectrum(const G4String& direction, const G4String& particle,
                          G4double energy, G4double weight)
{
  Histo& h = fFluxSpectra[direction][particle];
  ++h.nTotal;

  const G4int bin = FindBin(energy);
  if (bin < 0)       { ++h.nUnder; return; }
  if (bin >= kNBins) { ++h.nOver;  return; }

  h.weight[bin] += weight;
  h.weightSq[bin] += weight * weight;
  ++h.count[bin];
}

}
