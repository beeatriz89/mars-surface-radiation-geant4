// Author: Haegin Han
// Reference: ICRP Publication 145. Ann. ICRP 49(3), 2020.
// Geant4 Contributors: J. Allison and S. Guatelli

#include "TETDetectorConstruction.hh"
#include "G4Exception.hh"
#include "G4VisAttributes.hh"

TETDetectorConstruction::TETDetectorConstruction(TETModelImport* _tetData)
:fWorldPhysical(nullptr), fContainer_logic(nullptr), fTetData(_tetData), fTetLogic(nullptr)
{
 // initialisation of the variables for phantom information
 fPhantomSize     = fTetData -> GetPhantomSize();
 fPhantomBoxMin   = fTetData -> GetPhantomBoxMin();
 fPhantomBoxMax   = fTetData -> GetPhantomBoxMax();
 fNOfTetrahedrons = fTetData -> GetNumTetrahedron();
}

TETDetectorConstruction::~TETDetectorConstruction()
{
//  delete fTetData;
}

G4VPhysicalVolume* TETDetectorConstruction::Construct()
{
 SetupWorldGeometry();
 ConstructPhantom();
 PrintPhantomInformation();
 return nullptr;
}

void TETDetectorConstruction::SetupWorldGeometry()
{

    if (!fMotherVolume) {
        G4Exception("TETDetectorConstruction::SetupWorldGeometry",
                    "NoMotherVolume", FatalException,
                    "SetMotherVolume() must be called before Construct()!");
    }
 
    G4Material* vacuum =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
 
    // Phantom container box: bounding box + 10 cm margins on each side
    auto* containerSolid = new G4Box("phantomBox",
                                     fPhantomSize.x() / 2. + 10.*cm,
                                     fPhantomSize.y() / 2. + 10.*cm,
                                     fPhantomSize.z() / 2. + 10.*cm);
 
    fContainer_logic = new G4LogicalVolume(containerSolid, vacuum, "phantomLogical");
 
    // ---- Placement --------------------------------------------------------
    // The phantom stands on the Mars surface.
    // The surface is at z = +11 m (bottom of atmosphere layer 0).
    // The phantom box half-height is (fPhantomSize.z()/2 + 10 cm).
    // So the phantom centre is at:
    //   z_centre = z_surface + (fPhantomSize.z()/2 + 10 cm)
    // Adjust if your surface z-coordinate differs.
 
    G4double z_surface  = -36.26 * m;         // bottom of atmosphere (match DetectorConstruction)
    G4double halfHeight = fPhantomSize.z() / 2. + 5.*mm;
    G4double z_centre   = z_surface + halfHeight;
 
    G4cout << "[TETDetectorConstruction] Phantom container:"
           << "  size XYZ = " << fPhantomSize / mm << " mm"
           << "  centre z = " << z_centre / m << " m" << G4endl;
 
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0., 0., z_centre),
                      fContainer_logic,
                      "PhantomPhysical",
                      fMotherVolume,
                      false, 0, true);   // checkOverlaps=true


    fContainer_logic->SetOptimisation(TRUE);
    fContainer_logic->SetSmartless(0.5);

}

void TETDetectorConstruction::ConstructPhantom()
{

    // Template tetrahedron (shape only; actual vertices set by parameterisation)
    G4VSolid* tetraSolid = new G4Tet("TetSolid",
                                      G4ThreeVector(),
                                      G4ThreeVector(1.*cm, 0.,    0.),
                                      G4ThreeVector(0.,    1.*cm, 0.),
                                      G4ThreeVector(0.,    0.,    1.*cm));
 
    G4Material* vacuum =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
 
    // CRITICAL: logical volume name must be "TetLogic" — SteppingAction
    // checks lv->GetName() == "TetLogic" to identify phantom steps.
    fTetLogic = new G4LogicalVolume(tetraSolid, vacuum, "TetLogic");
 
    new G4PVParameterised("wholePhantom",
                          fTetLogic,
                          fContainer_logic,
                          kUndefined,
                          fTetData->GetNumTetrahedron(),
                          new TETParameterisation(fTetData));

}


// ---------------------------------------------------------------------------
// ConstructSDandField
//
// Called explicitly from DetectorConstruction::ConstructSDandField().
// In MT mode, Geant4 calls ConstructSDandField() on every worker thread;
// because DetectorConstruction is a real G4VUserDetectorConstruction,
// delegating here is sufficient for correct thread-local SD registration.
// ---------------------------------------------------------------------------


void TETDetectorConstruction::ConstructSDandField()
{

    // NOTE: We do NOT register a G4MultiFunctionalDetector here.
    //
    // Scoring is handled entirely by SteppingAction + OrganDosimetry.
    // Using both the SD primitive-scorer path AND SteppingAction would
    // double-count energy deposition.
    //
    // If you ever want to switch to the SD path (TETRun / TETPSEnergyDeposit),
    // remove the SteppingAction scoring and uncomment the block below.
    //
    // --- SD path (disabled) ------------------------------------------------
    // G4SDManager* pSDman = G4SDManager::GetSDMpointer();
    // auto* MFDet = new G4MultiFunctionalDetector("PhantomSD");
    // pSDman->AddNewDetector(MFDet);
    // MFDet->RegisterPrimitive(new TETPSEnergyDeposit("eDep", fTetData));
    // SetSensitiveDetector(fTetLogic, MFDet);
    // -----------------------------------------------------------------------

    G4cout << "[TETDetectorConstruction::ConstructSDandField] "
           << "SD path disabled — scoring via SteppingAction." << G4endl;

}

void TETDetectorConstruction::PrintPhantomInformation()
{

    G4cout << G4endl;
    G4cout.precision(3);
    G4cout << "   Phantom name               "
           << fTetData->GetPhantomName() << " TET phantom" << G4endl;
    G4cout << "   Phantom size (mm)          "
           << fPhantomSize.x() << " × "
           << fPhantomSize.y() << " × "
           << fPhantomSize.z() << G4endl;
    G4cout << "   Phantom box min (mm)       "
           << fPhantomBoxMin << G4endl;
    G4cout << "   Phantom box max (mm)       "
           << fPhantomBoxMax << G4endl;
    G4cout << "   Number of tetrahedrons     "
           << fNOfTetrahedrons << G4endl << G4endl;

}
