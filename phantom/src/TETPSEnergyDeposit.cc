// Author: Haegin Han
// Reference: ICRP Publication 145. Ann. ICRP 49(3), 2020.
// Geant4 Contributors: J. Allison and S. Guatelli

#include "TETPSEnergyDeposit.hh"

TETPSEnergyDeposit::TETPSEnergyDeposit(G4String name, TETModelImport* _tetData)
  :G4PSEnergyDeposit(name), fTetData(_tetData)
{}

G4int TETPSEnergyDeposit::GetIndex(G4Step* aStep)
{

  G4cout << ">>> ProcessHits called" << G4endl;

  // return the organ ID (= material index)
  G4int copyNo = aStep->GetPreStepPoint()->GetTouchable()->GetCopyNumber();
  return fTetData->GetMaterialIndex(copyNo);
}
