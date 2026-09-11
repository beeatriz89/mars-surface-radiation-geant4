#include "DetectorConstruction.hh"
#include "G4UserLimits.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Polyhedra.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

#include <cmath>


DetectorConstruction::DetectorConstruction()
{
  // ============================================================
  // SLAB
  // Área: 30 x 30 cm
  // Espessura total: 18 mm
  // ============================================================

  fSlabHalfX = 30 * cm;
  fSlabHalfY = 30 * cm;
  fSlabHalfZ = 9 * mm;
}


G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4NistManager* nist = G4NistManager::Instance();

  // ============================================================
  // Materiais
  // ============================================================

  fWaterMaterial = nist->FindOrBuildMaterial("G4_WATER");

  G4Material* worldMat =
      nist->FindOrBuildMaterial("G4_Galactic");


  // ============================================================
  // BC-430 / aproximação ao BC-432M2 do RAD
  //
  // rho = 1.023 g/cm3
  // H:C atomic ratio = 1.108
  // ============================================================

  G4double density = 1.023 * g / cm3;

  G4Material* bc432m2 =
      new G4Material("BC432M2", density, 2);

  G4Element* elH =
      nist->FindOrBuildElement("H");

  G4Element* elC =
      nist->FindOrBuildElement("C");

  bc432m2->AddElement(elH, 0.0851);
  bc432m2->AddElement(elC, 0.9149);


  // ============================================================
  // CsI(Tl)
  //
  // Para a simulação física, aproximamos o cristal como CsI.
  // O Tl é um dopante em concentração muito pequena.
  // ============================================================

  G4Element* elCs =
      nist->FindOrBuildElement("Cs");

  G4Element* elI =
      nist->FindOrBuildElement("I");

  G4Element* elTl =
      nist->FindOrBuildElement("Tl");

  // Densidade típica do CsI(Tl)
  G4double csiDensity = 4.51 * g / cm3;

  // CsI(Tl)
  //
  // A concentração de Tl é muito pequena e, para esta
  // aplicação dosimétrica, pode ser desprezada na composição
  // macroscópica. Assim usamos essencialmente CsI.
  G4Material* CsITl =
      new G4Material("CsI_Tl", csiDensity, 2);

  CsITl->AddElement(elCs, 0.5179);
  CsITl->AddElement(elI,  0.4821);


  // ============================================================
  // WORLD
  // ============================================================

  G4double pyramidHeight = 28 * mm;

  G4double worldHalf =
      1.5 * fSlabHalfX + 20.0 * cm;

  // Garantir também espaço suficiente acima do slab
  worldHalf = std::max(
      worldHalf,
      fSlabHalfZ + pyramidHeight + 20.0 * cm
  );

  G4Box* solidWorld =
      new G4Box(
          "World",
          worldHalf,
          worldHalf,
          worldHalf
      );

  G4LogicalVolume* logicWorld =
      new G4LogicalVolume(
          solidWorld,
          worldMat,
          "World"
      );

  logicWorld->SetVisAttributes(
      G4VisAttributes::GetInvisible()
  );

  G4VPhysicalVolume* physWorld =
      new G4PVPlacement(
          nullptr,
          G4ThreeVector(),
          logicWorld,
          "World",
          nullptr,
          false,
          0
      );


  // ============================================================
  // SLAB BC-432M2
  // ============================================================

  G4Box* solidSlab =
      new G4Box(
          "Slab",
          fSlabHalfX,
          fSlabHalfY,
          fSlabHalfZ
      );

  fLogicSlab =
      new G4LogicalVolume(
          solidSlab,
          bc432m2,
          "Slab"
      );


  G4double maxStep = 0.001 * mm;

  fLogicSlab->SetUserLimits(
      new G4UserLimits(maxStep)
  );


  new G4PVPlacement(
      nullptr,
      G4ThreeVector(0., 0., 0.),
      fLogicSlab,
      "Slab",
      logicWorld,
      false,
      0
  );


  // ============================================================
  // PIRÂMIDE QUADRADA CsI(Tl)
  //
  // Base: 30 x 30 cm
  // Altura: 28 mm
  //
  // A base fica colada à superfície superior do slab.
  //
  // ============================================================

  // G4Polyhedra usa um polígono regular como secção.
  //
  // Para 4 lados:
  //   raio circunscrito = lado / sqrt(2)
  //
  // Como o lado da base = 60 cm:
  //
  //   R = 60/sqrt(2) = 42.426 cm
  //
  G4double baseSide = 60 * cm;

  G4double baseRadius =
      baseSide / std::sqrt(2.0);


  // Duas secções ao longo de Z:
  //
  //  z = 0       -> base da pirâmide
  //  z = 28 mm   -> vértice
  //
  G4double zPlane[2] =
  {
      0.0,
      pyramidHeight
  };

  G4double rInner[2] =
  {
      0.0,
      0.0
  };

  G4double rOuter[2] =
  {
      baseRadius,
      0.0
  };


  G4Polyhedra* solidPyramid =
      new G4Polyhedra(
          "CsITlPyramid",
          45.0 * deg,
          360.0 * deg,
          4,
          2,
          zPlane,
          rInner,
          rOuter
      );


  G4LogicalVolume* logicPyramid =
      new G4LogicalVolume(
          solidPyramid,
          CsITl,
          "CsITlPyramid"
      );


  // A base da pirâmide deve coincidir com
  // a superfície superior do slab.
  //
  // Slab superior:
  //     z = +9 mm
  //
  // Portanto a pirâmide começa em:
  //     z = +9 mm
  //
  G4double pyramidBaseZ =
      fSlabHalfZ;


  new G4PVPlacement(
      nullptr,
      G4ThreeVector(
          0.0,
          0.0,
          pyramidBaseZ
      ),
      logicPyramid,
      "CsITlPyramid",
      logicWorld,
      false,
      0
  );


  // ============================================================
  // VISUALIZAÇÃO
  // ============================================================

  G4VisAttributes* slabVis =
      new G4VisAttributes();

  slabVis->SetVisibility(true);
  fLogicSlab->SetVisAttributes(slabVis);


  G4VisAttributes* pyramidVis =
      new G4VisAttributes();

  pyramidVis->SetVisibility(true);
  logicPyramid->SetVisAttributes(pyramidVis);


  return physWorld;
}


G4double DetectorConstruction::GetSlabMass() const
{
  // Massa do slab BC-432M2
  return fLogicSlab->GetMass();
}
