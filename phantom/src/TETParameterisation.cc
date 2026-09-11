// Author: Haegin Han
// Reference: ICRP Publication 145. Ann. ICRP 49(3), 2020.
// Geant4 Contributors: J. Allison and S. Guatelli
//
#include "TETParameterisation.hh"
#include "G4LogicalVolume.hh"
#include "G4VisExecutive.hh"
#include "G4RunManager.hh"

TETParameterisation::TETParameterisation(TETModelImport* _tetData)
: G4VPVParameterisation(), fTetData(_tetData)
{
 // initialise visAttMap which contains G4VisAttributes* for each organ
 auto colourMap =  fTetData->GetColourMap();
 for(auto colour : colourMap){	
     fVisAttMap[colour.first] = new G4VisAttributes(colour.second);
	}

 if(colourMap.size()) fIsforVis = true;
 else                 fIsforVis = false;
}

G4VSolid* TETParameterisation::ComputeSolid(
    		       const G4int copyNo, G4VPhysicalVolume* )
{
 return fTetData->GetTetrahedron(copyNo);
}

void TETParameterisation::ComputeTransformation(
                   const G4int,G4VPhysicalVolume*) const
{}

G4Material* TETParameterisation::ComputeMaterial(const G4int copyNo,
                                                 G4VPhysicalVolume* phy,
                                                 const G4VTouchable* )
{
   // set the colour for each organ if visualization is required
  if(fIsforVis){
	G4int idx = fTetData->GetMaterialIndex(copyNo);
	phy->GetLogicalVolume()->SetVisAttributes(fVisAttMap[idx]);
	phy->GetLogicalVolume()->SetMaterial(fTetData->GetMaterial(fTetData->GetMaterialIndex(copyNo)));
	}

// return the material data for each material index
return fTetData->GetMaterial(fTetData->GetMaterialIndex(copyNo));

}


