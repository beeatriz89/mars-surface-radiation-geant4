/// DetectorConstruction.cc  —  Mars Dosimetry Project

#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Element.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4GeometryManager.hh"
#include "G4SDManager.hh"
#include "G4VisExtent.hh"
#include "G4UserLimits.hh"
#include "G4Exception.hh"

#include <vector>
#include <string>

namespace B1
{

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
DetectorConstruction::DetectorConstruction(G4bool isAF)
: G4VUserDetectorConstruction(), fIsAF(isAF)
{
    // Build the TET data ONCE here (node/ele/material files are read once).
    // TETDetectorConstruction is created in Construct() after the world exists.
    fTetModelData = new TETModelImport(fIsAF, nullptr);
    G4cout << "[DetectorConstruction] TETModelImport loaded." << G4endl;
}

// ---------------------------------------------------------------------------
DetectorConstruction::~DetectorConstruction()
{
    delete fPhantom;
    delete fTetModelData;
}

// ---------------------------------------------------------------------------
// Helper: create a Martian atmospheric layer material
// ---------------------------------------------------------------------------
G4Material* DetectorConstruction::CreateAtmosMaterial(
    G4String name, G4double density, G4double temperature, G4double pressure)
{
    static G4NistManager* nist = G4NistManager::Instance();
    static G4Element* elC  = nist->FindOrBuildElement("C");
    static G4Element* elO  = nist->FindOrBuildElement("O");
    static G4Element* elN  = nist->FindOrBuildElement("N");
    static G4Element* elAr = nist->FindOrBuildElement("Ar");

    G4Material* mat = new G4Material(name, density, 4,
                                     kStateGas, temperature, pressure);
    // Mars atmosphere (mass fractions, CO2-dominated)
    mat->AddElement(elC,  0.138);
    mat->AddElement(elO,  0.738);
    mat->AddElement(elN,  0.053);
    mat->AddElement(elAr, 0.071);
    return mat;
}

// ---------------------------------------------------------------------------
void DetectorConstruction::DefineMaterials()
{
    G4cout << "\n==> Defining atmospheric and regolith materials..." << G4endl;

    // ---- Single atmospheric layer -----------------------------------------
    const G4int nCamadas = 1;
    G4double densidades[nCamadas]   = {0.021315621460765957};  // kg/m³
    G4double temperaturas[nCamadas] = {211.82208936438684};     // K
    G4double pressoes[nCamadas]     = {852.9060686234518};      // Pa

    fAtmosMateriais.clear();
    for (G4int i = 0; i < nCamadas; i++) {
        G4String nome = "Atmosfera_Camada_" + std::to_string(i);
        fAtmosMateriais.push_back(
            CreateAtmosMaterial(nome,
                                densidades[i]   * kg/m3,
                                temperaturas[i] * kelvin,
                                pressoes[i]     * pascal));
    }
    G4cout << "==> Created " << nCamadas << " atmospheric layer(s)." << G4endl;

    // ---- Martian regolith -------------------------------------------------
    G4NistManager* nist = G4NistManager::Instance();

    G4Element* elSi = nist->FindOrBuildElement("Si");
    G4Element* elAl = nist->FindOrBuildElement("Al");
    G4Element* elFe = nist->FindOrBuildElement("Fe");
    G4Element* elCa = nist->FindOrBuildElement("Ca");
    G4Element* elMg = nist->FindOrBuildElement("Mg");
    G4Element* elK  = nist->FindOrBuildElement("K");
    G4Element* elNa = nist->FindOrBuildElement("Na");
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elH  = nist->FindOrBuildElement("H");

    auto* SiO2 = new G4Material("SiO2_Mars",  2.65*g/cm3, 2);
    SiO2->AddElement(elSi, 1); SiO2->AddElement(elO, 2);

    auto* Fe2O3 = new G4Material("Fe2O3_Mars", 5.24*g/cm3, 2);
    Fe2O3->AddElement(elFe, 2); Fe2O3->AddElement(elO, 3);

    auto* H2O_mat = new G4Material("H2O_Mars",  1.*g/cm3, 2);
    H2O_mat->AddElement(elH, 2); H2O_mat->AddElement(elO, 1);

    auto* mixed = new G4Material("Al2CaK2MgNa2O7_Mars", 3.*g/cm3, 6);
    mixed->AddElement(elAl, 2); mixed->AddElement(elCa, 1);
    mixed->AddElement(elK,  2); mixed->AddElement(elMg, 1);
    mixed->AddElement(elNa, 2); mixed->AddElement(elO,  7);

    martianRegolith = new G4Material("MartianRegolith", 1.71*g/cm3, 4);
    martianRegolith->AddMaterial(SiO2,    51.2*perCent);
    martianRegolith->AddMaterial(H2O_mat,  7.4*perCent);
    martianRegolith->AddMaterial(mixed,   32.1*perCent);
    martianRegolith->AddMaterial(Fe2O3,    9.3*perCent);

    G4cout << "==> Martian regolith material created." << G4endl;
}

// ---------------------------------------------------------------------------
// Construct  — geometry (called once on master thread)
// ---------------------------------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4GeometryManager::GetInstance()->OpenGeometry();
    G4GeometryManager::GetInstance()->CloseGeometry(true, true);

    DefineMaterials();

    const G4bool checkOverlaps = true;
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* vacuum  = nist->FindOrBuildMaterial("G4_Galactic");

    // ---- World ------------------------------------------------------------
    auto* solidWorld = new G4Box("World", 10.*km, 10.*km, 10.*km);
    auto* logicWorld = new G4LogicalVolume(solidWorld, vacuum, "World");
    auto* physWorld  = new G4PVPlacement(nullptr, G4ThreeVector(),
                                         logicWorld, "World",
                                         nullptr, false, 0);

    // ---- Atmosphere (1 layer, cylinder) -----------------------------------
    // Layer 0: from z = 11 m (surface) to z = 83.7 m (top)
    G4double altitudes[2] = {83.72406537212937*m, 11.0*m};   // top, bottom
    G4double atmosRadius  = 1.*km;

    fAtmosLogicVolumes.clear();
    for (G4int i = 0; i < 1; i++) {
        G4double h_top    = altitudes[i];
        G4double h_bot    = altitudes[i+1];
        G4double h_half   = (h_top - h_bot) / 2.;
        G4double h_center = (h_top + h_bot) / 2.;

        G4String sName = "AtmosCamada_" + std::to_string(i) + "_solid";
        G4String lName = "AtmosCamada_" + std::to_string(i) + "_logic";
        G4String pName = "AtmosCamada_" + std::to_string(i);

        auto* solid = new G4Tubs(sName, 0., atmosRadius, h_half, 0.*deg, 360.*deg);
        auto* logic = new G4LogicalVolume(solid, fAtmosMateriais[i], lName);
        auto* phys  = new G4PVPlacement(nullptr,
                                        G4ThreeVector(0., 0., h_center),
                                        logic, pName, logicWorld,
                                        false, i, checkOverlaps);

        fAtmosLogicVolumes.push_back(logic);
        fAtmosPhysVolumes.push_back(phys);

        G4cout << "[DetectorConstruction] Atmosphere layer " << i
               << ": z_bot=" << h_bot/m << " m, z_top=" << h_top/m << " m" << G4endl;
    }

    // ---- Regolith (3 sub-layers) ------------------------------------------
    G4double reg_radius          = 1.*km;
    G4double reg_thickness_total = 10.*km;
    G4double reg_z_base          = -10.*km;
    G4double layer_thickness     = reg_thickness_total / 3.;
    G4double tolerancia          = 10.*nm;

    // z_positions[0..3]: bottom → top
    G4double z_positions[4] = {
        reg_z_base,
        reg_z_base +   layer_thickness,
        reg_z_base + 2.*layer_thickness,
        11.*m                           // surface
    };

    for (G4int i = 0; i < 3; i++) {
        // i=0 → topmost layer; i=2 → deepest
        G4double z_bot         = z_positions[2-i];
        G4double z_top         = z_positions[3-i];
        G4double z_center      = (z_bot + z_top) / 2.;
        G4double half_thickness = ((z_top - z_bot) / 2.) - tolerancia;

        G4String sName = "Regolith_Layer" + std::to_string(i) + "_solid";
        G4String lName = "Regolith_Layer" + std::to_string(i) + "_logic";
        G4String pName = "Regolith_Layer" + std::to_string(i);

        auto* solidReg = new G4Tubs(sName, 0., reg_radius, half_thickness,
                                    0.*deg, 360.*deg);
        auto* logicReg = new G4LogicalVolume(solidReg, martianRegolith, lName);
        new G4PVPlacement(nullptr, G4ThreeVector(0., 0., z_center),
                          logicReg, pName, logicWorld,
                          false, i, checkOverlaps);

        G4cout << "[DetectorConstruction] Regolith layer " << i
               << ": z_bot=" << z_bot/m << " m, z_top=" << z_top/m << " m" << G4endl;
    }

    // ---- TET Phantom ------------------------------------------------------
    // Build TETDetectorConstruction now that the world geometry exists.
    // The phantom is placed inside the lowest atmosphere layer (layer 0).
    // Its mother volume is the atmosphere logical volume so overlaps are
    // checked against the cylindrical atmosphere, not the huge world box.

    G4LogicalVolume* motherLV = fAtmosLogicVolumes[0];

    fPhantom = new TETDetectorConstruction(fTetModelData);
    fPhantom->SetMotherVolume(motherLV);
    fPhantom->Construct();   // fills fContainer_logic, fTetLogic

    G4cout << "[DetectorConstruction] TET phantom constructed." << G4endl;

    return physWorld;
}

// ---------------------------------------------------------------------------
// ConstructSDandField  — called by Geant4 MT on EVERY worker thread
// ---------------------------------------------------------------------------
void DetectorConstruction::ConstructSDandField()
{
    // Delegate to TETDetectorConstruction.
    // In the current scoring strategy (SteppingAction + OrganDosimetry),
    // no SD is registered — this is a no-op.
    // If you later switch to the SD/primitive-scorer path, uncomment
    // the relevant block in TETDetectorConstruction::ConstructSDandField().
    if (fPhantom) fPhantom->ConstructSDandField();
}

}
