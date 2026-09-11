// Author: Haegin Han
// Reference: ICRP Publication 145. Ann. ICRP 49(3), 2020.
// Geant4 Contributors: J. Allison and S. Guatelli

#ifndef TETModelImport_h
#define TETModelImport_h 1

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

#include "G4UIExecutive.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ThreeVector.hh"
#include "G4String.hh"
#include "G4Tet.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Colour.hh"

class TETModelImport
{
public:
 TETModelImport(G4bool isAF, G4UIExecutive* ui);
 virtual ~TETModelImport() {};
 
 // get methods
 G4String      GetPhantomName()           { return fPhantomName; };
 G4Material*   GetMaterial(G4int idx)     { return fMaterialMap[idx];}
 G4int         GetNumTetrahedron()        { return fTetVector.size();}
 G4int         GetMaterialIndex(G4int idx){ return fMaterialVector[idx]; }
 G4Tet*        GetTetrahedron(G4int idx)  { return fTetVector[idx]; }
 G4double      GetVolume(G4int idx)       { return fVolumeMap[idx]; }
 std::map<G4int, G4double> GetMassMap() const  { return fMassMap; }
 std::map<G4int, G4Colour> GetColourMap() { return fColourMap; }
 G4ThreeVector GetPhantomSize()           { return fPhantomSize; }
 G4ThreeVector GetPhantomBoxMin()         { return fBoundingBox_Min; }
 G4ThreeVector GetPhantomBoxMax()         { return fBoundingBox_Max; }
 std::map<G4int, G4String> GetOrganNameMap() const { return fOrganNameMap; }


private:
 // private methods
 void DataRead(G4String, G4String);
 void MaterialRead(G4String);
 void ColourRead();
 void PrintMaterialInfomation();

 G4String fPhantomDataPath;
 G4String fPhantomName;

 G4ThreeVector fBoundingBox_Min;
 G4ThreeVector fBoundingBox_Max;
 G4ThreeVector fPhantomSize;

 std::vector<G4ThreeVector> fVertexVector;
 std::vector<G4Tet*>        fTetVector;
 std::vector<G4int*>        fEleVector;
 std::vector<G4int>         fMaterialVector;
 std::map<G4int, G4int>     fNumTetMap;
 std::map<G4int, G4double>  fVolumeMap;
 std::map<G4int, G4double>  fMassMap;
 std::map<G4int, G4Colour>  fColourMap;

 std::map<G4int, std::vector<std::pair<G4int, G4double>>> fMaterialIndexMap;
 std::vector<G4int>                                       fMaterialIndex;
 std::map<G4int, G4Material*>                             fMaterialMap;
 std::map<G4int, G4double>                                fDensityMap;
 std::map<G4int, G4String>                                fOrganNameMap;
};

#endif
