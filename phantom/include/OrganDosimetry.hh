#pragma once
/// OrganDosimetry.hh  —  Mars Dosimetry Project
///
/// Scoring strategy:
///   • Per step: Accumulate() fills fEventMap (per matID, per event)
///   • Per event: EndOfEvent() flushes fEventMap into:
///       - fMap      : per {bucket, matID, dir}  — for per-matID outputs
///       - fGroupMap : per {bucket, groupName, dir} — for tissue-group σ
///   • Per run: WriteOutputs() computes D, H, E with correct uncertainties
///
/// Uncertainty for tissue groups:
///   σ²_T = (<x_T²> - <x_T>²) / N
///   where x_T(event) = Σᵢ∈group edep_i(event) — summed BEFORE squaring.
///   This is accumulated in fGroupMap.
///
/// Body mass for normalisation: fixed from organMasses_g (all matIDs),
/// independent of which matIDs had energy deposition.

#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleDefinition.hh"
#include "globals.hh"

#include <map>
#include <string>
#include <vector>
#include <set>

class OrganDosimetry
{
public:
    OrganDosimetry();
    void Reset();

    void Accumulate(G4int                       organID,
                    const G4ParticleDefinition* particle,
                    G4int                       particleZ,
                    G4int                       particleA,
                    G4double                    kineticEnergy,
                    G4double                    edep,
                    G4double                    stepLength,
                    G4double                    density,
                    const G4ThreeVector&        momentumDirection);

    // Flush per-step accumulators → per-event statistics.
    // Call from EventAction::EndOfEventAction().
    void EndOfEvent();

    void Merge(const OrganDosimetry& other);

    void WriteOutputs(const std::vector<G4double>& organMasses_g,
                      const std::vector<G4String>&  organNames,
                      const G4String&               outputPrefix,
                      G4int                         nPrimaries,
                      G4bool                        isAF = false) const;

    static G4double QualityFactor_ICRP60(G4double L_keV_um);
    static G4double QualityFactor_NASA(G4double L_keV_um, G4double beta,
                                   G4double E_per_nucleon_MeV,G4int Z);
    static G4double wR_reference(const std::string& bucket);
    static G4double wT(G4int id);   // kept for diagnostics; returns 0 (see .cc)

private:
    static std::string BucketName(const G4String& pname, G4int Z, G4int A);
    static G4double    LET_water_keV_um(const G4ParticleDefinition* p,
                                         G4double Ek_MeV);

    // ---- Keys --------------------------------------------------------------
    struct MatKey {
        std::string bucket;
        G4int       orgID;
        std::string dir;
        bool operator<(const MatKey& o) const {
            if (orgID  != o.orgID)  return orgID  < o.orgID;
            if (bucket != o.bucket) return bucket < o.bucket;
            return dir < o.dir;
        }
    };

    struct GroupKey {
        std::string bucket;
        std::string group;   // dosimetric tissue name, e.g. "Colon_wall"
        std::string dir;
        bool operator<(const GroupKey& o) const {
            if (group  != o.group)  return group  < o.group;
            if (bucket != o.bucket) return bucket < o.bucket;
            return dir < o.dir;
        }
    };

    // ---- Accumulators ------------------------------------------------------
    struct StepAccum {
        G4double edep  = 0.;   // [MeV]
        G4double hEdep = 0.;   // edep * Q_ICRP60
        G4double nEdep = 0.;   // edep * Q_NASA
    };

    struct DoseAccum {
        G4double edep_sum  = 0.;
        G4double hEdep_sum = 0.;
        G4double nEdep_sum = 0.;
        G4double edep_M2   = 0.;   // sum of x²  (for σ via sum/sum² method)
        G4double hEdep_M2  = 0.;
        G4double nEdep_M2  = 0.;
        G4long   nEvents   = 0;
    };

    // ---- Data --------------------------------------------------------------

    // Per-step buffer (flushed by EndOfEvent)
    std::map<MatKey,   StepAccum> fEventMap;

    // Per-matID permanent accumulator  → absorbed.out / equivalent.out detail
    std::map<MatKey,   DoseAccum> fMap;

    // Per-tissue-group permanent accumulator → correct group σ in effective.out
    // Key: {bucket, groupName, dir}
    // Accumulated as x_T(event) = Σᵢ∈group edep_i(event)
    std::map<GroupKey, DoseAccum> fGroupMap;

    // Tissue group definitions (built once in EndOfEvent / Reset)
    // matID → list of group names it belongs to
    // (populated lazily on first EndOfEvent call)
    mutable bool fGroupsBuilt = false;
    mutable std::map<G4int, std::vector<std::string>> fMatToGroups;
    void BuildGroupMap(G4bool isAF) const;   // fills fMatToGroups
};
