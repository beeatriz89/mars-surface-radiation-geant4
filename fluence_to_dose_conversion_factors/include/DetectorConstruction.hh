#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4Material.hh"

class G4LogicalVolume;

// Placa (slab) de tecido, centrada na origem, com a face de entrada
// virada para +z. A fonte fica acima (z > slabHalfZ) e o feixe viaja
// no sentido -z (incidência AP: antero-posterior).
class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;

    G4double GetSlabHalfX() const { return fSlabHalfX; }
    G4double GetSlabHalfY() const { return fSlabHalfY; }
    G4double GetSlabHalfZ() const { return fSlabHalfZ; }
    G4double GetSlabFrontZ() const { return fSlabHalfZ; } // face de entrada (+z)

    G4LogicalVolume* GetSlabLogicalVolume() const { return fLogicSlab; }
    G4Material* GetWaterMaterial() const { return fWaterMaterial; }
    G4double GetSlabMass() const; // massa da placa (kg, unidades internas G4)

  private:
    // Espessura total da placa = 18 mm  ->  meia-espessura = 9 mm
    G4double fSlabHalfX, fSlabHalfY, fSlabHalfZ;
    G4LogicalVolume* fLogicSlab = nullptr;
    G4Material* fWaterMaterial = nullptr;
};

#endif
