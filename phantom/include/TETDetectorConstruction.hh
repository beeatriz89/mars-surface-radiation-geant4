#pragma once
/// TETDetectorConstruction.hh  —  Mars Dosimetry Project
///
/// Wraps the ICRP-145 MRCP tetrahedral phantom.
/// Does NOT inherit G4VUserDetectorConstruction — it is called manually
/// from DetectorConstruction::Construct() after the world/atmosphere geometry
/// is built, and inserted as a daughter of the lowest atmosphere layer.
///
/// Key design points for ICRP-145 / MT:
///   • SetMotherVolume() must be called before Construct().
///   • ConstructSDandField() is called explicitly from
///     DetectorConstruction::ConstructSDandField() so Geant4's MT machinery
///     handles thread-local SD registration correctly.
///   • GetTetData() exposes the TETModelImport* so SteppingAction can
///     resolve copyNo → orgID without any string-based lookup.

#include "TETModelImport.hh"
#include "TETParameterisation.hh"
#include "TETPSEnergyDeposit.hh"

#include "G4VUserDetectorConstruction.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tet.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"
#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "globals.hh"

class TETDetectorConstruction
{
public:
    explicit TETDetectorConstruction(TETModelImport* tetData);
    ~TETDetectorConstruction();

    // Must be called before Construct()
    void SetMotherVolume(G4LogicalVolume* mother) { fMotherVolume = mother; }

    // Build phantom geometry into fMotherVolume
    G4VPhysicalVolume* Construct();

    // Called explicitly by DetectorConstruction::ConstructSDandField()
    void ConstructSDandField();

    // ---- Accessors --------------------------------------------------------
    G4LogicalVolume* GetContainerLogical() const { return fContainer_logic; }
    G4LogicalVolume* GetTetLogic()         const { return fTetLogic; }
    TETModelImport*  GetTetData()          const { return fTetData; }

    // Phantom bounding-box info (useful for geometry debugging)
    G4ThreeVector GetPhantomSize()   const { return fPhantomSize; }
    G4ThreeVector GetPhantomBoxMin() const { return fPhantomBoxMin; }
    G4ThreeVector GetPhantomBoxMax() const { return fPhantomBoxMax; }

private:
    void SetupWorldGeometry();
    void ConstructPhantom();
    void PrintPhantomInformation();

    // ---- Data members -----------------------------------------------------
    TETModelImport*  fTetData;          // non-owning
    G4LogicalVolume* fMotherVolume  = nullptr;
    G4LogicalVolume* fContainer_logic = nullptr;
    G4LogicalVolume* fTetLogic        = nullptr;
    G4VPhysicalVolume* fWorldPhysical = nullptr;  // unused but kept for compat

    G4ThreeVector fPhantomSize;
    G4ThreeVector fPhantomBoxMin;
    G4ThreeVector fPhantomBoxMax;
    G4int         fNOfTetrahedrons = 0;
};
