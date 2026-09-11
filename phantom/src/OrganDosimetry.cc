/// OrganDosimetry.cc  —  Mars Dosimetry Project

///
/// ICRP-103 effective dose with ICRP-145 MRCP phantoms.
///
/// Key design decisions:
///
///   1. H_T per ICRP-103 dosimetric tissue = ΣEdep_i·Q_i / Σmass_i
///      (energy-weighted, not average of individual H_i)
///
///   2. RBM and bone surface (endosteum) use site-specific mass fractions
///      from Table 3 of "Practical overview of ICRP-145 phantoms for MCNP".
///      spongiosa matIDs contain RBM + trabecular bone + yellow marrow;
///      only the RBM fraction contributes to wT=0.12 (red bone marrow).
///
///   3. wT assigned once per dosimetric tissue, not per matID.
///      Spongiosa matIDs return wT=0 from wT(); the RBM and bone-surface
///      H_T are computed explicitly in WriteOutputs using the Table-3 ratios.
///
///   4. Uncertainty (σ_D, σ_H) computed per PRIMARY EVENT, not per step.
///      EndOfEvent() flushes per-step accumulators into sum/sum² for
///      variance estimation: σ²_mean = (<x²> - <x>²) / N_primaries.
///
///   5. LET in water (ICRP-123 §100): computed via G4EmCalculator in G4_WATER.
///      For e-, e+, γ: Q=1 by definition (ICRP-123 §101), LET not needed.
///
///   6. Two quality factors: ICRP-60 Q(L) and NASA Q_NASA (Cucinotta et al.).

#include "OrganDosimetry.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4EmCalculator.hh"
#include "G4NistManager.hh"
#include "G4ParticleDefinition.hh"

#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <set>

// ============================================================================
// Static tissue group definitions
// Built once; same matIDs for AM and AF (sex-specific handled at wT level)
// ============================================================================
struct TissueGroup {
    std::string      name;
    G4double         wT_AM, wT_AF;
    std::vector<G4int> ids;
};

static const G4double wR = 0.12 / 13.0;

static const std::vector<TissueGroup> kGroups = {
    // wT = 0.12
    {"Stomach_wall",    0.12, 0.12, {7200,7201,7202,7203}},
    {"Colon_wall",      0.12, 0.12, {7600,7601,7602,
                                     7800,7801,7802,
                                     8000,8001,8002,
                                     8200,8201,8202,
                                     8400,8401,8402,
                                     8600}},
    {"Lung",            0.12, 0.12, {800,801,802,803,804,805,806,807,808,
                                     9700,9900}},
    // Breast: wT=0.12 for AF (meaningful); AM has negligible glandular tissue
    // but ICRP-103 still assigns wT=0.12 regardless of sex
    {"Breast_glandular",0.12, 0.12, {6300,6500}},
    // Gonads: testes (AM) / ovaries (AF) — wT=0.08
    // Both included; only one exists per sex
    {"Testes",          0.08, 0.0,  {12900,13000}},
    {"Ovaries",         0.0,  0.08, {11100,11200}},
    // wT = 0.04
    {"Liver",           0.04, 0.04, {9500}},
    {"Oesophagus",      0.04, 0.04, {11000,11001,11002}},
    {"Thyroid",         0.04, 0.04, {13200}},
    {"Urinary_bladder", 0.04, 0.04, {13700,13701}},
    // wT = 0.01
    {"Brain",           0.01, 0.01, {6100}},
    {"Salivary_glands", 0.01, 0.01, {12000,12100}},
    {"Skin",            0.01, 0.01, {12201,12301,12401,12501}},
    // Remainder wT = 0.12/13 each
    // AM: prostate; AF: uterus (different matID)
    {"Adrenals",        wR,   wR,   {100,200}},
    {"ET_region",       wR,   wR,   {300,301,302,303,
                                     400,401,402,403,404,405,700}},
    {"Gall_bladder",    wR,   wR,   {7000}},
    {"Heart_wall",      wR,   wR,   {8700}},
    {"Kidneys",         wR,   wR,   {8900,9000,9100,9200,9300,9400}},
    {"Lymph_nodes",     wR,   wR,   {10000,10100,10200,10300,10400,10500}},
    {"Muscle",          wR,   wR,   {10600,10700,10800,10900}},
    {"Oral_mucosa",     wR,   wR,   {500,501,600}},
    {"Pancreas",        wR,   wR,   {11300}},
    {"Small_intestine", wR,   wR,   {7400,7401,7402,7403}},
    {"Spleen",          wR,   wR,   {12700}},
    {"Thymus",          wR,   wR,   {13100}},
    // Sex-specific remainder
    {"Prostate",        wR,   0.0,  {11500}},        // AM only
    {"Uterus",          0.0,  wR,   {13900}},        // AF only
    // RBM and bone surface NOT in this table — computed separately with Table-3
};

// Build matID → group name(s) lookup (static, built once)
static std::map<G4int, std::vector<std::string>> BuildMatToGroups()
{
    std::map<G4int, std::vector<std::string>> m;
    for (const auto& grp : kGroups)
        for (G4int id : grp.ids)
            m[id].push_back(grp.name);
    return m;
}
static const std::map<G4int, std::vector<std::string>> kMatToGroups =
    BuildMatToGroups();

// ============================================================================
OrganDosimetry::OrganDosimetry() { Reset(); }

void OrganDosimetry::Reset()
{
    fMap.clear();
    fGroupMap.clear();
    fEventMap.clear();
}

// ============================================================================
std::string OrganDosimetry::BucketName(const G4String& pname, G4int pZ, G4int pA)
{
    if (pname == "e-")      return "electron";
    if (pname == "e+")      return "positron";
    if (pname == "gamma")   return "photon";
    if (pname == "neutron") return "neutron";
    if (pname == "mu-")     return "muon-";
    if (pname == "mu+")     return "muon+";
    if (pname == "pi-")     return "pion-";
    if (pname == "pi+")     return "pion+";
    if (pZ == 1 && pA == 1) return "proton";
    if (pZ == 1 && pA == 2) return "2H";
    if (pZ == 1 && pA == 3) return "3H";
    if (pZ == 2 && pA == 3) return "3He";
    if (pZ == 2 && pA == 4) return "alpha";
    if (pZ >= 3 && pZ <= 28) return "Z" + std::to_string(pZ);
    return "other";
}

// ============================================================================
G4double OrganDosimetry::LET_water_keV_um(const G4ParticleDefinition* p,
                                            G4double Ek_MeV)
{
    if (!p || Ek_MeV <= 0.) return 0.;
    const G4String& pn = p->GetParticleName();
    if (pn == "gamma" || pn == "e-" || pn == "e+") return 0.;
    static thread_local G4EmCalculator emCalc;
    static G4Material* water =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    if (!water) return 0.;

    G4double dEdx = emCalc.ComputeElectronicDEDX(Ek_MeV*MeV, p, water);
    return (dEdx > 0.) ? dEdx / (keV/um) : 0.;
}

// ============================================================================
G4double OrganDosimetry::QualityFactor_ICRP60(G4double L)
{
    if (L <= 0. || L < 10.) return 1.;
    if (L <= 100.) return 0.32*L - 2.2;
    return std::max(1., 300./std::sqrt(L));
}

// ============================================================================

G4double OrganDosimetry::QualityFactor_NASA(G4double L_keV_um,G4double beta,G4double E_per_nucleon_MeV,G4int Z)
{
    if (Z <= 0 || beta <= 0.0 ||
        L_keV_um <= 0.0 ||
        E_per_nucleon_MeV <= 0.0)
        return 1.0;

    // Effective charge Z*
    const G4double Z_d = static_cast<G4double>(Z);
    const G4double Z23 = std::pow(Z_d, 2.0 / 3.0);

    const G4double Zstar =
        Z_d * (1.0 - std::exp(-125.0 * beta / Z23));

    // NASA Q-model parameters: solid cancer
    constexpr G4double m = 3.0;
    const G4double kappa = (Z <= 4) ? 1000.0 : 500.0;

    // P(Z,E)
    const G4double trackTerm =
        1.0 - std::exp(
            -(Zstar * Zstar) /
            (kappa * beta * beta));

    const G4double energyTerm =
        1.0 - std::exp(-E_per_nucleon_MeV / 0.2);

    const G4double P =
        std::pow(trackTerm, m) * energyTerm;

    // NASA quality factor
    const G4double QNASA =
        (1.0 - P) +
        7000.0 * P / L_keV_um;

    return std::max(1.0, QNASA);

}

// ============================================================================
G4double OrganDosimetry::wT(G4int /*id*/) { return 0.; }

// ============================================================================
void OrganDosimetry::Accumulate(G4int organID,
                                 const G4ParticleDefinition* particle,
                                 G4int pZ, G4int pA,
                                 G4double kineticEnergy,
                                 G4double edep,
                                 G4double /*stepLength*/,
                                 G4double density,
                                 const G4ThreeVector& momDir)
{
    if (edep <= 0. || !particle) return;

    const G4double E_per_nucleon_MeV = (pA > 0) ? kineticEnergy / pA : kineticEnergy;
    const G4String& pn = particle->GetParticleName();
    const std::string dir    = (momDir.z() < 0.) ? "downward" : "upward";
    const std::string bucket = BucketName(pn, pZ, pA);
    const G4double edep_MeV  = edep / MeV;
    const G4double Ek_MeV    = kineticEnergy / MeV;
    const G4double rho        = density / (g/cm3);

    G4double L      = LET_water_keV_um(particle, Ek_MeV);
    G4double Q_icrp = (L > 0.) ? QualityFactor_ICRP60(L) : 1.;

    G4double beta = 0.;
    {

        // Rest mass in MeV
        G4double m0 = 0.;
        if      (pZ > 0 && pA > 0) m0 = pA * 931.494;   // nuclei
        else if (pn == "e-" || pn == "e+")   m0 = 0.511;
        else if (pn == "mu-"|| pn == "mu+")  m0 = 105.66;
        else if (pn == "pi-"|| pn == "pi+")  m0 = 139.57;
        else if (pn == "neutron")             m0 = 939.565;
        else if (pn == "proton")              m0 = 938.272;
        else                                  m0 = particle->GetPDGMass() / MeV;
        if (m0 > 0.) {
            G4double gamma = 1. + Ek_MeV / m0;
            beta = std::sqrt(1. - 1. / (gamma * gamma));

    }

  }

    G4double Q_nasa = (L > 0.) ? QualityFactor_NASA(L, beta, E_per_nucleon_MeV, pZ) : 1.;

    auto add = [&](const std::string& d) {
        StepAccum& sa = fEventMap[{bucket, organID, d}];
        sa.edep  += edep_MeV;
        sa.hEdep += edep_MeV * Q_icrp;
        sa.nEdep += edep_MeV * Q_nasa;
    };
    add(dir);
    add("all");
}

// ============================================================================
// EndOfEvent
// Flush fEventMap into:
//   fMap      — per {bucket, matID, dir}
//   fGroupMap — per {bucket, groupName, dir}  with correct x_T² accumulation
// ============================================================================
void OrganDosimetry::EndOfEvent()
{
    if (fEventMap.empty()) return;

    // --- Update fMap (per matID) ---
    for (auto& [key, sa] : fEventMap) {
        if (sa.edep <= 0.) continue;
        DoseAccum& da = fMap[key];
        da.nEvents++;
        da.edep_sum  += sa.edep;
        da.hEdep_sum += sa.hEdep;
        da.nEdep_sum += sa.nEdep;
        da.edep_M2   += sa.edep  * sa.edep;
        da.hEdep_M2  += sa.hEdep * sa.hEdep;
        da.nEdep_M2  += sa.nEdep * sa.nEdep;
    }

    // --- Update fGroupMap (per tissue group) ---
    // First, aggregate x_T(event) = Σᵢ∈group edep_i(event)
    // Key: {bucket, groupName, dir}
    struct GroupEventKey {
        std::string bucket, group, dir;
        bool operator<(const GroupEventKey& o) const {
            if (group  != o.group)  return group  < o.group;
            if (bucket != o.bucket) return bucket < o.bucket;
            return dir < o.dir;
        }
    };
    std::map<GroupEventKey, StepAccum> groupEvent;

    for (auto& [key, sa] : fEventMap) {
        if (sa.edep <= 0.) continue;
        auto it = kMatToGroups.find(key.orgID);
        if (it == kMatToGroups.end()) continue;
        for (const auto& gname : it->second) {
            StepAccum& ge = groupEvent[{key.bucket, gname, key.dir}];
            ge.edep  += sa.edep;
            ge.hEdep += sa.hEdep;
            ge.nEdep += sa.nEdep;
        }
    }

    // Now flush groupEvent into fGroupMap with correct x_T² accumulation
    for (auto& [gkey, ge] : groupEvent) {
        if (ge.edep <= 0.) continue;
        DoseAccum& da = fGroupMap[{gkey.bucket, gkey.group, gkey.dir}];
        da.nEvents++;
        da.edep_sum  += ge.edep;
        da.hEdep_sum += ge.hEdep;
        da.nEdep_sum += ge.nEdep;
        da.edep_M2   += ge.edep  * ge.edep;   // x_T² — summed after grouping
        da.hEdep_M2  += ge.hEdep * ge.hEdep;
        da.nEdep_M2  += ge.nEdep * ge.nEdep;
    }

    fEventMap.clear();
}

// ============================================================================
void OrganDosimetry::Merge(const OrganDosimetry& other)
{
    for (const auto& [key, da] : other.fMap) {
        DoseAccum& dst = fMap[key];
        dst.edep_sum  += da.edep_sum;  dst.hEdep_sum += da.hEdep_sum;
        dst.nEdep_sum += da.nEdep_sum; dst.edep_M2   += da.edep_M2;
        dst.hEdep_M2  += da.hEdep_M2;  dst.nEdep_M2  += da.nEdep_M2;
        dst.nEvents   += da.nEvents;
    }
    for (const auto& [key, da] : other.fGroupMap) {
        DoseAccum& dst = fGroupMap[key];
        dst.edep_sum  += da.edep_sum;  dst.hEdep_sum += da.hEdep_sum;
        dst.nEdep_sum += da.nEdep_sum; dst.edep_M2   += da.edep_M2;
        dst.hEdep_M2  += da.hEdep_M2;  dst.nEdep_M2  += da.nEdep_M2;
        dst.nEvents   += da.nEvents;
    }
}

// ============================================================================
void OrganDosimetry::WriteOutputs(const std::vector<G4double>& organMasses_g,
                                   const std::vector<G4String>& organNames,
                                   const G4String& prefix,
                                   G4int nPrimaries,
                                   G4bool isAF) const
{
    constexpr G4double MeV_to_J = 1.602176634e-13;
    const G4double     N        = static_cast<G4double>(nPrimaries);

    // ---- Fixed body mass (ALL matIDs, not just activeOrgs) -----------------
    G4double bodyMass_kg = 0.;
    for (G4int id = 0; id < (G4int)organMasses_g.size(); ++id)
        bodyMass_kg += organMasses_g[id] * 1e-3;

    // ---- Helpers -----------------------------------------------------------
    auto mass_kg = [&](G4int id) -> G4double {
        return (id>=0 && id<(G4int)organMasses_g.size())
               ? organMasses_g[id]*1e-3 : 0.;
    };
    auto orgName = [&](G4int id) -> G4String {
        return (id>=0 && id<(G4int)organNames.size())
               ? organNames[id] : G4String("unknown");
    };

    // σ_mean from sum and sum² (per primary event):
    // σ² = (<x²> - <x>²) / N = (M2/N - (sum/N)²) / N
    auto sigma = [&](G4double sum, G4double M2, G4double mass) -> G4double {
        if (mass<=0.||N<=0.) return 0.;
        G4double mean = sum/N;
        G4double var  = std::max(0., M2/N - mean*mean)/N;
        return std::sqrt(var)*MeV_to_J/mass;
    };

    // Sum edep/hEdep/nEdep over all buckets for a matID
    auto sumMat = [&](G4int id, const std::string& dir)
        -> std::tuple<G4double,G4double,G4double>
    {
        G4double se=0.,si=0.,sn=0.;
        for (const auto& [k,da] : fMap)
            if (k.orgID==id && k.dir==dir) { se+=da.edep_sum; si+=da.hEdep_sum; sn+=da.nEdep_sum; }
        return {se,si,sn};
    };

    // Active matIDs
    std::set<G4int> activeOrgs;
    for (const auto& [k,v] : fMap) activeOrgs.insert(k.orgID);

    // Bucket order
    const std::vector<std::string> prefOrder = {
        "proton","2H","3H","3He","alpha",
        "Z3","Z4","Z5","Z6","Z7","Z8","Z9","Z10",
        "Z11","Z12","Z13","Z14","Z15","Z16","Z17","Z18",
        "Z19","Z20","Z21","Z22","Z23","Z24","Z25","Z26","Z27","Z28",
        "electron","positron","photon","neutron",
        "muon-","muon+","pion-","pion+","other"
    };
    std::set<std::string> seenB;
    for (const auto& [k,v] : fMap) seenB.insert(k.bucket);
    std::vector<std::string> buckets;
    for (auto& b : prefOrder) if (seenB.count(b)) buckets.push_back(b);
    for (auto& b : seenB)
        if (!std::count(prefOrder.begin(),prefOrder.end(),b)) buckets.push_back(b);

    const std::vector<std::string> dirs = {"downward","upward","all"};

    // ---- Bone data (Table 3) -----------------------------------------------
    struct BoneSite { G4int sp,med,cort; G4double rbm_AM,rbm_AF,esp_AM,esp_AF,emed_AM,emed_AF; };
    static const BoneSite kB[] = {
        {1400,1500,1300, 26.9,20.7, 9.41,7.16, 0.19,0.14},
        {1700,1800,1600,  0.0, 0.0,11.25,8.32, 0.25,0.19},
        {2000,2100,1900,  0.0, 0.0,16.31,12.03,0.09,0.07},
        {2300,  -1,2200,  0.0, 0.0,12.50,7.10, 0.00,0.00},
        {2500,  -1,2400,  9.3, 7.2, 2.50,1.90, 0.00,0.00},
        {2700,  -1,2600, 88.9,68.4,83.40,64.20,0.00,0.00},
        {2900,3000,2800, 78.4,60.3,43.34,33.53,0.86,0.67},
        {3200,3300,3100,  0.0, 0.0,47.83,23.67,0.67,0.33},
        {3500,3600,3400,  0.0, 0.0,87.38,79.91,5.02,4.59},
        {3800,  -1,3700,  0.0, 0.0,42.20,24.40,0.00,0.00},
        {4000,  -1,3900,  9.4, 7.2, 2.00,1.60, 0.00,0.00},
        {4200,  -1,4100,205.2,157.5,51.70,39.70,0.00,0.00},
        {4400,  -1,4300,188.8,144.9,29.80,22.90,0.00,0.00},
        {4600,  -1,4500, 32.8,25.2, 9.80,7.60, 0.00,0.00},
        {4800,  -1,4700, 45.6,35.1,11.50,8.80, 0.00,0.00},
        {5000,  -1,4900,188.8,144.9,26.90,20.60,0.00,0.00},
        {5200,  -1,5100,143.9,110.7,23.40,18.00,0.00,0.00},
        {5400,  -1,5300,115.9,89.1,20.60,15.80,0.00,0.00},
        {5600,  -1,5500, 36.3,27.9, 5.50,4.30, 0.00,0.00},
    };
    const G4int nB = sizeof(kB)/sizeof(kB[0]);

    G4double totalRBM=0., totalEndo=0.;
    for (G4int i=0;i<nB;++i) {
        totalRBM  += isAF ? kB[i].rbm_AF : kB[i].rbm_AM;
        totalEndo += isAF ? kB[i].esp_AF+kB[i].emed_AF
                          : kB[i].esp_AM+kB[i].emed_AM;
    }

    // Helper: H_T for a bone component (RBM or endosteum)
    // Returns {H_icrp, H_nasa, sigH_icrp, sigH_nasa}
    //
    // Since bone sites are independent volumes, the variance of the
    // weighted sum is the sum of weighted variances:
    //   σ²_RBM = Σᵢ (wᵢ/total)² × σ²_H_sp(i)
    // where σ²_H_sp(i) = (<h²> - <h>²) / N  for spongiosa site i.

    auto boneHT = [&](bool doRBM) -> std::tuple<G4double,G4double,G4double,G4double>
    {
        G4double Hi=0.,Hn=0.,sig2i=0.,sig2n=0.;
        G4double total=(doRBM?totalRBM:totalEndo);
        if (total<=0.) return {0.,0.,0.,0.};
 
        for (G4int i=0;i<nB;++i) {
            G4double wsp  = doRBM ? (isAF?kB[i].rbm_AF:kB[i].rbm_AM)
                                  : (isAF?kB[i].esp_AF:kB[i].esp_AM);
            G4double wmed = doRBM ? 0. : (isAF?kB[i].emed_AF:kB[i].emed_AM);
            G4double f_sp = wsp / total;   // weight fraction for this site
 
            // Spongiosa contribution
            G4double mk_sp = mass_kg(kB[i].sp);
            if (mk_sp > 0. && wsp > 0.) {
                // Sum hEdep over all buckets for this matID
                G4double sumHi=0.,sumHn=0.,sumH2i=0.,sumH2n=0.;
                for (const auto& bkt : buckets) {
                    auto it = fMap.find({bkt, kB[i].sp, "all"});
                    if (it != fMap.end()) {
                        sumHi  += it->second.hEdep_sum;
                        sumHn  += it->second.nEdep_sum;
                        sumH2i += it->second.hEdep_M2;
                        sumH2n += it->second.nEdep_M2;
                    }
                }
                G4double H_sp_i = sumHi * MeV_to_J / (mk_sp * N);
                G4double H_sp_n = sumHn * MeV_to_J / (mk_sp * N);
                Hi += f_sp * H_sp_i;
                Hn += f_sp * H_sp_n;
                // Variance contribution: (f_sp)² × σ²_H_sp
                if (N > 0.) {
                    G4double var_i = std::max(0., sumH2i/N - (sumHi/N)*(sumHi/N)) / N;
                    G4double var_n = std::max(0., sumH2n/N - (sumHn/N)*(sumHn/N)) / N;
                    sig2i += f_sp * f_sp * var_i * (MeV_to_J/mk_sp) * (MeV_to_J/mk_sp);
                    sig2n += f_sp * f_sp * var_n * (MeV_to_J/mk_sp) * (MeV_to_J/mk_sp);
                }
            }
 
            // Medullary cavity contribution (endosteum only)
            if (!doRBM && kB[i].med >= 0 && wmed > 0.) {
                G4double f_med  = wmed / total;
                G4double mk_med = mass_kg(kB[i].med);
                if (mk_med > 0.) {
                    G4double sumHi=0.,sumHn=0.,sumH2i=0.,sumH2n=0.;
                    for (const auto& bkt : buckets) {
                        auto it = fMap.find({bkt, kB[i].med, "all"});
                        if (it != fMap.end()) {
                            sumHi  += it->second.hEdep_sum;
                            sumHn  += it->second.nEdep_sum;
                            sumH2i += it->second.hEdep_M2;
                            sumH2n += it->second.nEdep_M2;
                        }
                    }
                    Hi += f_med * sumHi * MeV_to_J / (mk_med * N);
                    Hn += f_med * sumHn * MeV_to_J / (mk_med * N);
                    if (N > 0.) {
                        G4double var_i = std::max(0., sumH2i/N - (sumHi/N)*(sumHi/N)) / N;
                        G4double var_n = std::max(0., sumH2n/N - (sumHn/N)*(sumHn/N)) / N;
                        sig2i += f_med*f_med * var_i * (MeV_to_J/mk_med)*(MeV_to_J/mk_med);
                        sig2n += f_med*f_med * var_n * (MeV_to_J/mk_med)*(MeV_to_J/mk_med);
                    }
                }
            }
        }
        return {Hi, Hn, std::sqrt(sig2i), std::sqrt(sig2n)};
    };
 
    G4cout << "[OrganDosimetry] Writing outputs: sex=" << (isAF?"AF":"AM")
           << "  activeOrgs=" << activeOrgs.size()
           << "  N=" << nPrimaries
           << "  bodyMass=" << bodyMass_kg*1e3 << " g" << G4endl;
 
    // =========================================================================
    // FILE 1 — _absorbed.out
    // =========================================================================
    {
        std::ofstream f(prefix+"_absorbed.out");
        f << "# Absorbed Dose D [Gy/source]  N=" << nPrimaries
          << "  sex=" << (isAF?"AF":"AM") << "\n"
          << "# sigma_D: per-event uncertainty (sum/sum² method)\n"
          << "# D_T (tissue groups): mass-weighted sum over sub-organs\n#\n";
 
        // Per-matID detail
        f << std::string(125,'#') << "\n"
          << "# " << std::setw(6)  << "matID"
          << "  " << std::setw(32) << std::left  << "Organ" << std::right
          << "  " << std::setw(12) << "Particle"
          << "  " << std::setw(10) << "Direction"
          << "  " << std::setw(15) << "Edep[MeV]"
          << "  " << std::setw(15) << "D[Gy/src]"
          << "  " << std::setw(15) << "sigD[Gy/src]"
          << "\n" << std::string(125,'#') << "\n";
 
        std::map<G4int,G4double> matD;
        for (G4int id : activeOrgs) {
            G4double mk=mass_kg(id);
            G4String nm=orgName(id);
            for (const auto& bkt:buckets) {
                for (const auto& dir:dirs) {
                    auto it=fMap.find({bkt,id,dir});
                    if (it==fMap.end()) continue;
                    const DoseAccum& da=it->second;
                    if (da.edep_sum<=0.) continue;
                    G4double D=0.,sD=0.;
                    if (mk>0.&&N>0.) { D=da.edep_sum*MeV_to_J/(mk*N); sD=sigma(da.edep_sum,da.edep_M2,mk); }
                    f << "  " << std::setw(6) << id
                      << "  " << std::setw(32) << std::left << nm.substr(0,32) << std::right
                      << "  " << std::setw(12) << bkt
                      << "  " << std::setw(10) << dir
                      << "  " << std::setw(15) << std::scientific << std::setprecision(6) << da.edep_sum
                      << "  " << std::setw(15) << D
                      << "  " << std::setw(15) << sD << "\n";
                    if (dir=="all") matD[id]+=D;
                }
            }
        }
 
        f << "\n# ---- D per matID (all particles, all directions) ----\n";
        G4double grand=0.;
        for (auto& [id,D]:matD) {
            f << "  " << std::setw(6) << id
              << "  " << std::setw(32) << std::left << orgName(id).substr(0,32) << std::right
              << "  " << std::setw(15) << std::scientific << std::setprecision(6) << D << "\n";
            grand+=D;
        }
        f << "# Grand total D = " << grand << " Gy/src\n";
 
        // Per tissue group D_T
        f << "\n# ---- D_T per dosimetric tissue (ICRP-103, mass-weighted) ----\n"
          << "# " << std::setw(20) << std::left << "Tissue" << std::right
          << "  " << std::setw(9)  << "wT"
          << "  " << std::setw(15) << "D_T[Gy/src]"
          << "  " << std::setw(15) << "sigD_T[Gy/src]\n";
        for (const auto& grp:kGroups) {
            G4double gwT = isAF ? grp.wT_AF : grp.wT_AM;
            // D_T = Σ edep_i / (Σ mass_i * N)
            G4double sumE=0.,sumM=0.;
            for (G4int id:grp.ids) {
                auto [se,si,sn]=sumMat(id,"all"); sumE+=se; sumM+=mass_kg(id);
            }
            if (sumE<=0.||sumM<=0.) continue;
            G4double DT=sumE*MeV_to_J/(sumM*N);
            // σ_T from fGroupMap
            G4double sTot=0.,s2Tot=0.;
            for (const auto& bkt:buckets) {
                auto it=fGroupMap.find({bkt,grp.name,"all"});
                if (it!=fGroupMap.end()) { sTot+=it->second.edep_sum; s2Tot+=it->second.edep_M2; }
            }
            G4double sigDT=sigma(sTot,s2Tot,sumM);
            f << "  " << std::setw(20) << std::left << grp.name << std::right
              << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << gwT
              << "  " << std::setw(15) << std::scientific << std::setprecision(6) << DT
              << "  " << std::setw(15) << sigDT << "\n";
        }
        // RBM and bone
        auto [Drbm,_Dr1,sigDrbm_i,sigDrbm_n]=boneHT(true);
        auto [Dbone,_Db1,sigDbone_i,sigDbone_n]=boneHT(false);
        f << "  " << std::setw(20) << std::left << "Red_bone_marrow" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.12
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Drbm
          << "  " << std::setw(15) << sigDrbm_i << "\n";
        f << "  " << std::setw(20) << std::left << "Bone_surface" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.01
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Dbone
          << "  " << std::setw(15) << sigDbone_i << "\n";
 
        // Per-particle D (fixed body mass)
        f << "\n# ---- D per particle (body average, fixed bodyMass=" 
          << std::fixed << std::setprecision(1) << bodyMass_kg*1e3 << " g) ----\n"
          << "# " << std::setw(12) << "Particle"
          << "  " << std::setw(15) << "Edep_tot[MeV]"
          << "  " << std::setw(15) << "D_body[Gy/src]"
          << "  " << std::setw(10) << "fraction\n";
        G4double totE=0.;
        std::map<std::string,G4double> eByBkt;
        for (G4int id:activeOrgs) for (const auto& bkt:buckets) {
            auto it=fMap.find({bkt,id,"all"});
            if (it!=fMap.end()) { eByBkt[bkt]+=it->second.edep_sum; totE+=it->second.edep_sum; }
        }
        for (const auto& bkt:buckets) {
            if (eByBkt[bkt]<=0.) continue;
            G4double Db=(bodyMass_kg>0.)?eByBkt[bkt]*MeV_to_J/(bodyMass_kg*N):0.;
            f << "  " << std::setw(12) << bkt
              << "  " << std::setw(15) << std::scientific << std::setprecision(6) << eByBkt[bkt]
              << "  " << std::setw(15) << Db
              << "  " << std::setw(10) << std::fixed << std::setprecision(4)
              << (totE>0.?eByBkt[bkt]/totE:0.) << "\n";
        }
        f.close();
        G4cout<<"[OrganDosimetry] Written "<<prefix+"_absorbed.out"<<G4endl;
    }
 
    // =========================================================================
    // FILE 2 — _equivalent.out
    // =========================================================================
    {
        std::ofstream f(prefix+"_equivalent.out");
        f << "# Equivalent Dose H [Sv/source]  N=" << nPrimaries
          << "  sex=" << (isAF?"AF":"AM") << "\n"
          << "# H_ICRP: Q(L) ICRP-60/123 in water\n"
          << "# H_NASA: Q_NASA Cucinotta et al.\n"
          << "# H_T (tissue groups): mass-weighted, sigma from per-event group accumulation\n#\n";
 
        f << std::string(140,'#') << "\n"
          << "# " << std::setw(6)  << "matID"
          << "  " << std::setw(32) << std::left  << "Organ" << std::right
          << "  " << std::setw(12) << "Particle"
          << "  " << std::setw(10) << "Direction"
          << "  " << std::setw(15) << "H_ICRP[Sv/src]"
          << "  " << std::setw(12) << "sigH_ICRP"
          << "  " << std::setw(7)  << "Q_ICRP"
          << "  " << std::setw(15) << "H_NASA[Sv/src]"
          << "  " << std::setw(12) << "sigH_NASA"
          << "  " << std::setw(7)  << "Q_NASA"
          << "\n" << std::string(140,'#') << "\n";
 
        for (G4int id:activeOrgs) {
            G4double mk=mass_kg(id); G4String nm=orgName(id);
            for (const auto& bkt:buckets) for (const auto& dir:dirs) {
                auto it=fMap.find({bkt,id,dir});
                if (it==fMap.end()) continue;
                const DoseAccum& da=it->second;
                if (da.edep_sum<=0.) continue;
                G4double H=0.,Hn=0.,sH=0.,sN=0.,Qi=1.,Qn=1.;
                if (mk>0.&&N>0.) {
                    H =da.hEdep_sum*MeV_to_J/(mk*N);
                    Hn=da.nEdep_sum*MeV_to_J/(mk*N);
                    sH=sigma(da.hEdep_sum,da.hEdep_M2,mk);
                    sN=sigma(da.nEdep_sum,da.nEdep_M2,mk);
                    if (da.edep_sum>0.){Qi=da.hEdep_sum/da.edep_sum;Qn=da.nEdep_sum/da.edep_sum;}
                }
                f << "  " << std::setw(6) << id
                  << "  " << std::setw(32) << std::left << nm.substr(0,32) << std::right
                  << "  " << std::setw(12) << bkt
                  << "  " << std::setw(10) << dir
                  << "  " << std::setw(15) << std::scientific << std::setprecision(6) << H
                  << "  " << std::setw(12) << sH
                  << "  " << std::setw(7)  << std::fixed << std::setprecision(2) << Qi
                  << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Hn
                  << "  " << std::setw(12) << sN
                  << "  " << std::setw(7)  << Qn << "\n";
            }
        }
 
        // H_T per tissue group
        f << "\n# ---- H_T per dosimetric tissue ----\n"
          << "# " << std::setw(20) << std::left << "Tissue" << std::right
          << "  " << std::setw(9)  << "wT"
          << "  " << std::setw(15) << "H_T_ICRP[Sv]"
          << "  " << std::setw(12) << "sigH_ICRP"
          << "  " << std::setw(15) << "H_T_NASA[Sv]"
          << "  " << std::setw(12) << "sigH_NASA\n";
        for (const auto& grp:kGroups) {
            G4double gwT=isAF?grp.wT_AF:grp.wT_AM;
            G4double sumHi=0.,sumHn=0.,sumM=0.;
            G4double sTi=0.,s2Ti=0.,sTn=0.,s2Tn=0.;
            for (G4int id:grp.ids) {
                auto [se,si,sn]=sumMat(id,"all");
                sumHi+=si; sumHn+=sn; sumM+=mass_kg(id);
            }
            if (sumHi<=0.||sumM<=0.) continue;
            G4double HT_i=sumHi*MeV_to_J/(sumM*N);
            G4double HT_n=sumHn*MeV_to_J/(sumM*N);
            // σ from fGroupMap (correct: x_T summed before squaring)
            for (const auto& bkt:buckets) {
                auto iti=fGroupMap.find({bkt,grp.name,"all"});
                if (iti!=fGroupMap.end()){ sTi+=iti->second.hEdep_sum; s2Ti+=iti->second.hEdep_M2; sTn+=iti->second.nEdep_sum; s2Tn+=iti->second.nEdep_M2; }
            }
            G4double sigHi=sigma(sTi,s2Ti,sumM);
            G4double sigHn=sigma(sTn,s2Tn,sumM);
            f << "  " << std::setw(20) << std::left << grp.name << std::right
              << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << gwT
              << "  " << std::setw(15) << std::scientific << std::setprecision(6) << HT_i
              << "  " << std::setw(12) << sigHi
              << "  " << std::setw(15) << HT_n
              << "  " << std::setw(12) << sigHn << "\n";
        }
        auto [Hrbm_i,Hrbm_n,sigHrbm_i,sigHrbm_n]=boneHT(true);
        auto [Hbone_i,Hbone_n,sigHbone_i,sigHbone_n]=boneHT(false);
        f << "  " << std::setw(20) << std::left << "Red_bone_marrow" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.12
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Hrbm_i
          << "  " << std::setw(12) << sigHrbm_i
          << "  " << std::setw(15) << Hrbm_n
          << "  " << std::setw(12) << sigHrbm_n << "\n";
        f << "  " << std::setw(20) << std::left << "Bone_surface" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.01
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Hbone_i
          << "  " << std::setw(12) << sigHbone_i
          << "  " << std::setw(15) << Hbone_n
          << "  " << std::setw(12) << sigHbone_n << "\n";
 
        // Per-particle H (fixed body mass)
        f << "\n# ---- H per particle (body average, fixed bodyMass="
          << std::fixed << std::setprecision(1) << bodyMass_kg*1e3 << " g) ----\n"
          << "# " << std::setw(12) << "Particle"
          << "  " << std::setw(15) << "H_ICRP[Sv/src]"
          << "  " << std::setw(15) << "H_NASA[Sv/src]"
          << "  " << std::setw(10) << "Q_eff_ICRP"
          << "  " << std::setw(10) << "Q_eff_NASA\n";
        for (const auto& bkt:buckets) {
            G4double sH=0.,sN=0.,sE=0.;
            for (G4int id:activeOrgs) {
                auto it=fMap.find({bkt,id,"all"});
                if (it!=fMap.end()){sH+=it->second.hEdep_sum;sN+=it->second.nEdep_sum;sE+=it->second.edep_sum;}
            }
            if (sE<=0.) continue;
            f << "  " << std::setw(12) << bkt
              << "  " << std::setw(15) << std::scientific << std::setprecision(6)
              << (bodyMass_kg>0.?sH*MeV_to_J/(bodyMass_kg*N):0.)
              << "  " << std::setw(15) << (bodyMass_kg>0.?sN*MeV_to_J/(bodyMass_kg*N):0.)
              << "  " << std::setw(10) << std::fixed << std::setprecision(3) << sH/sE
              << "  " << std::setw(10) << sN/sE << "\n";
        }
        f.close();
        G4cout<<"[OrganDosimetry] Written "<<prefix+"_equivalent.out"<<G4endl;
    }
 
    // =========================================================================
    // FILE 3 — _effective.out
    // =========================================================================
    {
        std::ofstream f(prefix+"_effective.out");
        f << "# Effective Dose E [Sv/source] — ICRP-103 (2007)\n"
          << "# H_T mass-weighted; RBM/bone with Table-3 fractions\n"
          << "# sex=" << (isAF?"AF":"AM") << "  N=" << nPrimaries << "\n#\n"
          << std::string(110,'#') << "\n"
          << "# " << std::setw(20) << std::left << "Tissue" << std::right
          << "  " << std::setw(9)  << "wT"
          << "  " << std::setw(15) << "H_T_ICRP[Sv]"
          << "  " << std::setw(15) << "wT*H_ICRP"
          << "  " << std::setw(15) << "H_T_NASA[Sv]"
          << "  " << std::setw(15) << "wT*H_NASA"
          << "\n" << std::string(110,'#') << "\n";
 
        G4double E_icrp=0., E_nasa=0.;
 
        for (const auto& grp:kGroups) {
            G4double gwT=isAF?grp.wT_AF:grp.wT_AM;
            if (gwT<=0.) continue;
            G4double sumHi=0.,sumHn=0.,sumM=0.;
            for (G4int id:grp.ids) {
                auto [se,si,sn]=sumMat(id,"all");
                sumHi+=si; sumHn+=sn; sumM+=mass_kg(id);
            }
            if (sumHi<=0.||sumM<=0.) continue;
            G4double HT_i=sumHi*MeV_to_J/(sumM*N);
            G4double HT_n=sumHn*MeV_to_J/(sumM*N);
            f << "  " << std::setw(20) << std::left << grp.name << std::right
              << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << gwT
              << "  " << std::setw(15) << std::scientific << std::setprecision(6) << HT_i
              << "  " << std::setw(15) << gwT*HT_i
              << "  " << std::setw(15) << HT_n
              << "  " << std::setw(15) << gwT*HT_n << "\n";
            E_icrp+=gwT*HT_i; E_nasa+=gwT*HT_n;
        }
 
        // RBM and bone
        auto [Hri,Hrn,sigHri,sigHrn]=boneHT(true);
        auto [Hbi,Hbn,sigHbi,sigHbn]=boneHT(false);
        f << "  " << std::setw(20) << std::left << "Red_bone_marrow" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.12
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Hri
          << "  " << std::setw(15) << 0.12*Hri
          << "  " << std::setw(15) << Hrn
          << "  " << std::setw(15) << 0.12*Hrn << "\n";
        f << "  " << std::setw(20) << std::left << "Bone_surface" << std::right
          << "  " << std::setw(9)  << std::fixed << std::setprecision(6) << 0.01
          << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Hbi
          << "  " << std::setw(15) << 0.01*Hbi
          << "  " << std::setw(15) << Hbn
          << "  " << std::setw(15) << 0.01*Hbn << "\n";
        E_icrp+=0.12*Hri+0.01*Hbi;
        E_nasa +=0.12*Hrn+0.01*Hbn;
 
        f << "\n# " << std::string(80,'=') << "\n"
          << "# TOTAL E (ICRP-60 Q) = " << std::scientific << E_icrp << " Sv/src\n"
          << "# TOTAL E (NASA    Q) = " << std::scientific << E_nasa << " Sv/src\n"
          << "# " << std::string(80,'=') << "\n";
 
        // Per-particle E
        f << "\n# ---- E by particle ----\n"
          << "# " << std::setw(12) << "Particle"
          << "  " << std::setw(15) << "E_ICRP[Sv/src]"
          << "  " << std::setw(15) << "E_NASA[Sv/src]\n";
        for (const auto& bkt:buckets) {
            G4double Ei=0.,En=0.;
            for (const auto& grp:kGroups) {
                G4double gwT=isAF?grp.wT_AF:grp.wT_AM;
                if (gwT<=0.) continue;
                G4double sHi=0.,sHn=0.,sM=0.;
                for (G4int id:grp.ids) {
                    auto it=fMap.find({bkt,id,"all"});
                    if (it!=fMap.end()){sHi+=it->second.hEdep_sum;sHn+=it->second.nEdep_sum;}
                    sM+=mass_kg(id);
                }
                if (sM>0.&&N>0.){Ei+=gwT*sHi*MeV_to_J/(sM*N);En+=gwT*sHn*MeV_to_J/(sM*N);}
            }
            // Bone contribution per particle
            for (G4int i=0;i<nB;++i) {
                G4double rbm=isAF?kB[i].rbm_AF:kB[i].rbm_AM;
                G4double esp=isAF?kB[i].esp_AF:kB[i].esp_AM;
                G4double emed=isAF?kB[i].emed_AF:kB[i].emed_AM;
                G4double mk_sp=mass_kg(kB[i].sp);
                if (mk_sp>0.) {
                    auto it=fMap.find({bkt,kB[i].sp,"all"});
                    if (it!=fMap.end()) {
                        G4double hi=it->second.hEdep_sum,hn=it->second.nEdep_sum;
                        if (rbm>0.&&totalRBM>0.){Ei+=0.12*(rbm/totalRBM)*hi*MeV_to_J/(mk_sp*N);En+=0.12*(rbm/totalRBM)*hn*MeV_to_J/(mk_sp*N);}
                        if (esp>0.&&totalEndo>0.){Ei+=0.01*(esp/totalEndo)*hi*MeV_to_J/(mk_sp*N);En+=0.01*(esp/totalEndo)*hn*MeV_to_J/(mk_sp*N);}
                    }
                }
                if (kB[i].med>=0&&emed>0.&&totalEndo>0.) {
                    G4double mk_m=mass_kg(kB[i].med);
                    if (mk_m>0.) {
                        auto it=fMap.find({bkt,kB[i].med,"all"});
                        if (it!=fMap.end()){
                            Ei+=0.01*(emed/totalEndo)*it->second.hEdep_sum*MeV_to_J/(mk_m*N);
                            En+=0.01*(emed/totalEndo)*it->second.nEdep_sum*MeV_to_J/(mk_m*N);
                        }
                    }
                }
            }
            if (Ei>0.)
                f << "  " << std::setw(12) << bkt
                  << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Ei
                  << "  " << std::setw(15) << En << "\n";
        }
 
        // Per-direction E
        f << "\n# ---- E by direction ----\n"
          << "# " << std::setw(12) << "Direction"
          << "  " << std::setw(15) << "E_ICRP[Sv/src]"
          << "  " << std::setw(15) << "E_NASA[Sv/src]\n";
        for (const auto& dir:dirs) {
            G4double Ei=0.,En=0.;
            for (const auto& grp:kGroups) {
                G4double gwT=isAF?grp.wT_AF:grp.wT_AM;
                if (gwT<=0.) continue;
                G4double sHi=0.,sHn=0.,sM=0.;
                for (G4int id:grp.ids) {
                    auto [se,si,sn]=sumMat(id,dir);
                    sHi+=si; sHn+=sn; sM+=mass_kg(id);
                }
                if (sM>0.&&N>0.){Ei+=gwT*sHi*MeV_to_J/(sM*N);En+=gwT*sHn*MeV_to_J/(sM*N);}
            }
            if (Ei>0.)
                f << "  " << std::setw(12) << dir
                  << "  " << std::setw(15) << std::scientific << std::setprecision(6) << Ei
                  << "  " << std::setw(15) << En << "\n";
        }
 
        f.close();
        G4cout<<"[OrganDosimetry] Written "<<prefix+"_effective.out"<<G4endl;
    }
}
 
G4double OrganDosimetry::wR_reference(const std::string& bkt)
{
    if (bkt=="proton")  return 2.;
    if (bkt=="alpha")   return 20.;
    if (bkt=="neutron") return 10.;
    return 1.;
}
