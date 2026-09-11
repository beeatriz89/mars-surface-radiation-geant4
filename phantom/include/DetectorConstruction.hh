#pragma once
/// DetectorConstruction.hh  —  Mars Dosimetry Project

#include "G4VUserDetectorConstruction.hh"
#include "G4Material.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "globals.hh"

#include "TETDetectorConstruction.hh"
#include "TETModelImport.hh"

#include <vector>

namespace B1
{

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    explicit DetectorConstruction(G4bool isAF = true);
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

    // Called by Geant4 MT on every worker thread for SD registration
    void ConstructSDandField() override;

    // Accessor used by SteppingAction to retrieve TETModelImport*
    const TETDetectorConstruction* GetPhantom() const { return fPhantom; }

private:
    // ---- Material helpers -------------------------------------------------
    void      DefineMaterials();
    G4Material* CreateAtmosMaterial(G4String name,
                                    G4double density,
                                    G4double temperature,
                                    G4double pressure);

    // ---- Data members -----------------------------------------------------
    G4bool   fIsAF = true;

    // Atmosphere
    std::vector<G4Material*>       fAtmosMateriais;
    std::vector<G4LogicalVolume*>  fAtmosLogicVolumes;
    std::vector<G4VPhysicalVolume*> fAtmosPhysVolumes;

    // Regolith
    G4Material* martianRegolith = nullptr;

    // Phantom
    TETModelImport*           fTetModelData = nullptr;   // owned
    TETDetectorConstruction*  fPhantom      = nullptr;   // owned
};

}
