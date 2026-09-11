// Author: Haegin Han
// Reference: ICRP Publication 145. Ann. ICRP 49(3), 2020.
// Geant4 Contributors: J. Allison and S. Guatelli
//

#include "TETRun.hh"

TETRun::TETRun()
:G4Run(), fCollID(-1)
{}

TETRun::~TETRun()
{
 fEdepMap.clear();
}

void TETRun::RecordEvent(const G4Event* event)
{
 if(fCollID<0)
  fCollID = G4SDManager::GetSDMpointer()->GetCollectionID("PhantomSD/eDep");

 // Hits collections
 //
 G4HCofThisEvent* HCE = event->GetHCofThisEvent();
 if(!HCE) return;
 
 auto* evtMap = static_cast<G4THitsMap<G4double>*>(HCE->GetHC(fCollID));
 // sum up the energy deposition and the square of it
 for (auto itr : *evtMap->GetMap()) {
		fEdepMap[itr.first].first  += *itr.second;                   //sum
		fEdepMap[itr.first].second += (*itr.second) * (*itr.second); //sum square
	}
}

void TETRun::Merge(const G4Run* run)
{
 // merge the data from each thread
 EDEPMAP localMap = static_cast<const TETRun*>(run)->fEdepMap;

 for(auto itr : localMap){
	 fEdepMap[itr.first].first  += itr.second.first;
         fEdepMap[itr.first].second += itr.second.second;
	}

 G4Run::Merge(run);
}





