/////////////////////////////////////////////////////////////////////////////
/// Class:       MCTruthT0Matching
/// Module Type: producer
/// File:        MCTruthT0Matching_module.cc
///
/// Author:         Thomas Karl Warburton
/// E-mail address: k.warburton@sheffield.ac.uk
///
/// Generated at Wed Mar 25 13:54:28 2015 by Thomas Warburton using artmod
/// from cetpkgsupport v1_08_04.
///
/// This module accesses the Monte Carlo Truth information stored in the ART
/// event and matches that with a track. It does this by looping through the
/// tracks in the event and looping through each hit in the track.
/// For each hit it uses the backtracker service to work out the charge which
/// each MCTruth particle contributed to the total charge desposited for the
/// hit.
/// The MCTruth particle which is ultimately assigned to the track is simply
/// the particle which deposited the most charge.
/// It then stores an ART anab::T0 object which has the following variables;
/// 1) Generation time of the MCTruth particle assigned to track, in ns.
/// 2) The trigger type used to assign T0 (in this case 2 for MCTruth)
/// 3) The Geant4 TrackID of the particle (so can access all MCTrtuh info in
///     subsequent modules).
/// 4) The track number of this track in this event.
///
/// The module has been extended to also associate an anab::T0 object with a
/// recob::Shower. It does this following the same algorithm, where
/// recob::Track has been replaced with recob::Shower.
///
/// The module takes a reconstructed track as input.
/// The module outputs an anab::T0 object
///
/// * Update (25 Oct 2017) --- wketchum@fnal.gov *
///   --Add option for storing hit, MCParticle associations.
///
/// * Update (6 Nov 2017) --- yuntse@slac.stanford.edu
///   --Add a few variables in the metadata of hit, MCParticle associations.
///
/////////////////////////////////////////////////////////////////////////////

// Framework includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Event.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art_root_io/TFileService.h"
#include "art_root_io/TFileDirectory.h"

#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>
#include <iostream>
#include <map>
#include <iterator>

// LArSoft
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/AnalysisBase/T0.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/RawData/ExternalTrigger.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "lardataobj/AnalysisBase/BackTrackerMatchingData.h"

// ROOT
#include "TTree.h"
#include "TFile.h"

namespace t0 {
  class MCTruthT0Matching;
}

class t0::MCTruthT0Matching : public art::EDProducer {
public:
  explicit MCTruthT0Matching(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MCTruthT0Matching(MCTruthT0Matching const &) = delete;
  MCTruthT0Matching(MCTruthT0Matching &&) = delete;
  MCTruthT0Matching & operator = (MCTruthT0Matching const &) = delete;
  MCTruthT0Matching & operator = (MCTruthT0Matching &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // Selected optional functions.
  void beginJob() override;

private:

  // Params got from fcl file
  art::InputTag fTrackModuleLabel;
  art::InputTag fShowerModuleLabel;
  art::InputTag fPFParticleModuleLabel;
  bool fMakeT0Assns;
  bool fMakePFParticleAssns;

  art::InputTag fHitModuleLabel;
  bool fMakeHitAssns;

  bool fOverrideRealData;

  // Variable in TFS branches
  TTree* fTree;
  int    TrackID         = 0;
  int    TrueTrackID     = 0;
  int    TrueTriggerType = 0;
  double TrueTrackT0     = 0;

  int    ShowerID          = 0;
  int    ShowerMatchID     = 0;
  int    ShowerTriggerType = 0;
  double ShowerT0          = 0;
};


t0::MCTruthT0Matching::MCTruthT0Matching(fhicl::ParameterSet const & p)
  : EDProducer{p}
{
  // Call appropriate produces<>() functions here.
  fTrackModuleLabel  = (p.get< art::InputTag > ("TrackModuleLabel" ) );
  fShowerModuleLabel = (p.get< art::InputTag > ("ShowerModuleLabel") );
  fPFParticleModuleLabel = (p.get< art::InputTag > ("PFParticleModuleLabel", "pandoraNu") );
  fMakeT0Assns = (p.get< bool > ("makeT0Assns", true) );
  fMakePFParticleAssns =  (p.get< bool > ("makePFParticleAssns", false) );

  fMakeHitAssns = p.get<bool>("makeHitAssns",true);
  if(fMakeHitAssns) fHitModuleLabel = p.get<art::InputTag>("HitModuleLabel");
  fOverrideRealData = p.get<bool>("OverrideRealData",false);

  if (fMakeT0Assns){ // T0 assns are deprecated - this allows one to use deprecated funcionality. Added 2017-08-15. Should not be kept around forever
    std::cout << "WARNING - You are using deprecated functionality\n";
    std::cout << "MCTruthT0Matching T0 assns will be removed soon\n";
    std::cout << "set your fcl parameter makeT0Assns to false and use MCParticle direct associations instead" << std::endl;
    produces< std::vector<anab::T0>               >();
    produces< art::Assns<recob::Track , anab::T0> >();
    produces< art::Assns<recob::Shower, anab::T0> > ();
    if (fMakePFParticleAssns){produces< art::Assns<recob::PFParticle, anab::T0> > ();}// only do PFParticles if desired by the user
  }

  produces< art::Assns<recob::Track , simb::MCParticle, anab::BackTrackerMatchingData > > ();
  produces< art::Assns<recob::Shower, simb::MCParticle, anab::BackTrackerMatchingData > > ();
  if (fMakePFParticleAssns){ // only do PFParticles if desired by the user
    produces< art::Assns<recob::PFParticle, simb::MCParticle, anab::BackTrackerMatchingData > > ();
  }

  if(fMakeHitAssns)
    produces< art::Assns<recob::Hit , simb::MCParticle, anab::BackTrackerHitMatchingData > > ();

}

void t0::MCTruthT0Matching::beginJob()
{
  // Implementation of optional member function here.
  art::ServiceHandle<art::TFileService const> tfs;
  fTree = tfs->make<TTree>("MCTruthT0Matching","MCTruthT0");
  fTree->Branch("TrueTrackT0",&TrueTrackT0,"TrueTrackT0/D");
  fTree->Branch("TrueTrackID",&TrueTrackID,"TrueTrackID/D");
  //fTree->Branch("ShowerID"   ,&ShowerID   ,"ShowerID/I"   );
  //fTree->Branch("ShowerT0"   ,&ShowerT0   ,"ShowerT0/D"   );
}

void t0::MCTruthT0Matching::produce(art::Event & evt)
{
  if (evt.isRealData() && !fOverrideRealData) return;

  // Access art services...
  art::ServiceHandle<geo::Geometry const> geom;
  art::ServiceHandle<cheat::BackTrackerService const> bt_serv;
  art::ServiceHandle<cheat::ParticleInventoryService const> pi_serv;

  //TrackList handle
  art::Handle< std::vector<recob::Track> > trackListHandle;
  std::vector<art::Ptr<recob::Track> > tracklist;
  if (evt.getByLabel(fTrackModuleLabel,trackListHandle))
    art::fill_ptr_vector(tracklist, trackListHandle);
  //if (!trackListHandle.isValid()) trackListHandle.clear();

  //ShowerList handle
  art::Handle< std::vector<recob::Shower> > showerListHandle;
  std::vector<art::Ptr<recob::Shower> > showerlist;
  if (evt.getByLabel(fShowerModuleLabel,showerListHandle))
    art::fill_ptr_vector(showerlist, showerListHandle);
  //if (!showerListHandle.isValid()) showerListHandle.clear();

  //PFParticleList handle
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfparticlelist;
  if (evt.getByLabel(fPFParticleModuleLabel,pfparticleListHandle))
    art::fill_ptr_vector(pfparticlelist, pfparticleListHandle);
  //if (!pfparticleListHandle.isValid()) pfparticleListHandle.clear();


  auto mcpartHandle = evt.getValidHandle< std::vector<simb::MCParticle> >("largeant");
//  simb::MCParticle const *firstParticle = &mcpartHandle->front();
  // Create anab::T0 objects and make association with recob::Track, recob::Shower, and recob::PFParticle objects
  std::unique_ptr< std::vector<anab::T0> > T0col( new std::vector<anab::T0>);
//  if (fMakeT0Assns){ // T0 assns are deprecated - this allows one to use deprecated funcionality. Added 2017-08-15. Should not be kept around forever
    std::unique_ptr< art::Assns<recob::Track, anab::T0> > Trackassn( new art::Assns<recob::Track, anab::T0>);
    std::unique_ptr< art::Assns<recob::Shower, anab::T0> > Showerassn( new art::Assns<recob::Shower, anab::T0>);
//    if (fMakePFParticleAssns){
      std::unique_ptr< art::Assns<recob::PFParticle, anab::T0> > PFParticleassn( new art::Assns<recob::PFParticle, anab::T0>);
//    }
//  }

  // Create associations directly between MCParticle and either recob::Track, recob::Shower, or recob::PFParticle
  // These associations contain metadata summarising the quality of the match
  std::unique_ptr< art::Assns<recob::Track, simb::MCParticle, anab::BackTrackerMatchingData > > MCPartTrackassn( new art::Assns<recob::Track, simb::MCParticle, anab::BackTrackerMatchingData >);
  std::unique_ptr< art::Assns<recob::Shower, simb::MCParticle, anab::BackTrackerMatchingData> > MCPartShowerassn( new art::Assns<recob::Shower, simb::MCParticle, anab::BackTrackerMatchingData>);
//  if (fMakePFParticleAssns){
    std::unique_ptr< art::Assns<recob::PFParticle, simb::MCParticle,anab::BackTrackerMatchingData> > MCPartPFParticleassn( new art::Assns<recob::PFParticle, simb::MCParticle, anab::BackTrackerMatchingData>);
//  }
   // Association block for the hits<-->MCParticles
      std::unique_ptr< art::Assns<recob::Hit, simb::MCParticle, anab::BackTrackerHitMatchingData > > MCPartHitassn( new art::Assns<recob::Hit, simb::MCParticle, anab::BackTrackerHitMatchingData >);


    double maxe = -1;
    double tote = 0;
   // int    trkid = -1;
    int    maxtrkid = -1;
    double maxn = -1;
    double totn = 0;
    int maxntrkid = -1;
    anab::BackTrackerHitMatchingData bthmd;

    std::unordered_map<int,int> trkid_lookup; //indexed by geant4trkid, delivers MC particle location

    //if we want to make per-hit assns
    if(fMakeHitAssns){



      art::Handle< std::vector<recob::Hit> > hitListHandle;
      evt.getByLabel(fHitModuleLabel,hitListHandle);

      if(hitListHandle.isValid()){

	auto const& hitList(*hitListHandle);
	auto const& mcpartList(*mcpartHandle);
	for(size_t i_h=0; i_h<hitList.size(); ++i_h){
	  art::Ptr<recob::Hit> hitPtr(hitListHandle, i_h);
	  auto trkide_list = bt_serv->HitToTrackIDEs(hitPtr);
          struct TrackIDEinfo {
            float E;
            float NumElectrons;
          };
	  std::map<int, TrackIDEinfo> trkide_collector;
	  maxe = -1; tote = 0; maxtrkid = -1;
          maxn = -1; totn = 0; maxntrkid = -1;
	  for(auto const& t : trkide_list){
	    trkide_collector[t.trackID].E += t.energy;
	    tote += t.energy;
	    if(trkide_collector[t.trackID].E>maxe) { maxe = trkide_collector[t.trackID].E; maxtrkid = t.trackID; }
            trkide_collector[t.trackID].NumElectrons += t.numElectrons;
            totn += t.numElectrons;
            if(trkide_collector[t.trackID].NumElectrons > maxn) {
              maxn = trkide_collector[t.trackID].NumElectrons;
              maxntrkid = t.trackID;
            }

	    //if not found, find mc particle...
	    if(trkid_lookup.find(t.trackID)==trkid_lookup.end()){
	      size_t i_p=0;
	      while(i_p<mcpartList.size()){
		if(mcpartList[i_p].TrackId() == t.trackID) { trkid_lookup[t.trackID] = (int)i_p; break;}
		++i_p;
	      }
	      if(i_p==mcpartList.size()) trkid_lookup[t.trackID] = -1;
	    }

	  }//end loop on TrackIDs

	  //now find the mcparticle and loop back through ...
	  for(auto const& t : trkide_collector){
	    int mcpart_i = trkid_lookup[t.first];
	    if(mcpart_i==-1) continue; //no mcparticle here
	    art::Ptr<simb::MCParticle> mcpartPtr(mcpartHandle, mcpart_i);
	    bthmd.ideFraction = t.second.E / tote;
	    bthmd.isMaxIDE = (t.first==maxtrkid);
            bthmd.ideNFraction = t.second.NumElectrons / totn;
            bthmd.isMaxIDEN = ( t.first == maxntrkid );
	    MCPartHitassn->addSingle(hitPtr, mcpartPtr, bthmd);
	  }


	}//end loop on hits

      }//end if handle valid

    }//end if make hit/mcpart assns


    if (trackListHandle.isValid()){
      //Access tracks and hits
      art::FindManyP<recob::Hit> fmtht(trackListHandle, evt, fTrackModuleLabel);

      size_t NTracks = tracklist.size();

      // Now to access MCTruth for each track...
      for(size_t iTrk=0; iTrk < NTracks; ++iTrk) {
	TrueTrackT0 = 0;
	TrackID     = 0;
	TrueTrackID = 0;
	anab::BackTrackerMatchingData btdata;
	std::vector< art::Ptr<recob::Hit> > allHits = fmtht.at(iTrk);

	std::map<int,double> trkide;
	for(size_t h = 0; h < allHits.size(); ++h){
	  art::Ptr<recob::Hit> hit = allHits[h];
	  std::vector<sim::IDE> ides;
	  std::vector<sim::TrackIDE> TrackIDs = bt_serv->HitToTrackIDEs(hit);

	  for(size_t e = 0; e < TrackIDs.size(); ++e){
	    trkide[TrackIDs[e].trackID] += TrackIDs[e].energy;
	  }

	}
	// Work out which IDE despoited the most charge in the hit if there was more than one.
	maxe = -1;
	tote = 0;
      for (std::map<int,double>::iterator ii = trkide.begin(); ii!=trkide.end(); ++ii){
	tote += ii->second;
	if ((ii->second)>maxe){
	  maxe = ii->second;
	  TrackID = ii->first;
	}
      }
      btdata.cleanliness = maxe/tote;

      // Now have trackID, so get PdG code and T0 etc.
      const simb::MCParticle *tmpParticle = pi_serv->TrackIdToParticle_P(TrackID);
      if (!tmpParticle) continue; // Retain this check that the BackTracker can find the right particle
      // Now, loop through the MCParticle's myself to find the correct match
      int mcpart_i(-1);
      for (auto const particle : *mcpartHandle){
        mcpart_i++;
        if (TrackID == particle.TrackId()){
          break;
        }
      }
      const simb::MCParticle particle = mcpartHandle.product()->at(mcpart_i);
      TrueTrackT0 = particle.T();
      TrueTrackID = particle.TrackId();
      TrueTriggerType = 2; // Using MCTruth as trigger, so tigger type is 2.
      //std::cout << "Got particle, PDG = " << particle.PdgCode() << std::endl;

      //std::cout << "Filling T0col with " << TrueTrackT0 << " " << TrueTriggerType << " " << TrueTrackID << " " << (*T0col).size() << std::endl;

      T0col->push_back(anab::T0(TrueTrackT0,
				TrueTriggerType,
				TrueTrackID,
				(*T0col).size()
				));
      //auto diff = particle - firstParticle;
      auto diff = mcpart_i; // check I have a sensible value for this counter
      if (diff >= (int)mcpartHandle->size()){
        std::cout << "Error, the backtracker is doing weird things to your pointers!" << std::endl;
        throw std::exception();
      }

      art::Ptr<simb::MCParticle> mcpartPtr(mcpartHandle, mcpart_i);
      MCPartTrackassn->addSingle(tracklist[iTrk], mcpartPtr, btdata);
      if (fMakeT0Assns){
        util::CreateAssn(*this, evt, *T0col, tracklist[iTrk], *Trackassn);
      }
      fTree -> Fill();

    } // Loop over tracks
  }

  if (showerListHandle.isValid()){
    art::FindManyP<recob::Hit> fmsht(showerListHandle,evt, fShowerModuleLabel);
    // Now Loop over showers....
    size_t NShowers = showerlist.size();
    for (size_t Shower = 0; Shower < NShowers; ++Shower) {
      ShowerMatchID     = 0;
      ShowerID          = 0;
      ShowerT0          = 0;
      std::vector< art::Ptr<recob::Hit> > allHits = fmsht.at(Shower);
      anab::BackTrackerMatchingData btdata;

      std::map<int,double> showeride;
      for(size_t h = 0; h < allHits.size(); ++h){
	art::Ptr<recob::Hit> hit = allHits[h];
	std::vector<sim::IDE> ides;
	std::vector<sim::TrackIDE> TrackIDs = bt_serv->HitToTrackIDEs(hit);

	for(size_t e = 0; e < TrackIDs.size(); ++e){
	  showeride[TrackIDs[e].trackID] += TrackIDs[e].energy;
	}
      }
      // Work out which IDE despoited the most charge in the hit if there was more than one.
      maxe = -1;
      tote = 0;
      for (std::map<int,double>::iterator ii = showeride.begin(); ii!=showeride.end(); ++ii){
	tote += ii->second;
	if ((ii->second)>maxe){
	  maxe = ii->second;
	  ShowerID = ii->first;
	}
      }
      btdata.cleanliness = maxe/tote;




      // Now have MCParticle trackID corresponding to shower, so get PdG code and T0 etc.
      const simb::MCParticle *tmpParticle = pi_serv->TrackIdToParticle_P(ShowerID);
      if (!tmpParticle) continue; // Retain this check that the BackTracker can find the right particle
      // Now, loop through the MCParticle's myself to find the correct match
      int mcpart_i(-1);
      for (auto const particle : *mcpartHandle){
        mcpart_i++;
        if (ShowerID == particle.TrackId()){
          break;
        }
      }
      const simb::MCParticle particle = mcpartHandle.product()->at(mcpart_i);
      ShowerT0 = particle.T();
      ShowerID = particle.TrackId();
      ShowerTriggerType = 2; // Using MCTruth as trigger, so tigger type is 2.
      T0col->push_back(anab::T0(ShowerT0,
				ShowerTriggerType,
				ShowerID,
				(*T0col).size()
				));

      auto diff = mcpart_i; // check I have a sensible value for this counter
      if (diff >= (int)mcpartHandle->size()){
        std::cout << "Error, the backtracker is doing weird things to your pointers!" << std::endl;
        throw std::exception();
      }
      art::Ptr<simb::MCParticle> mcpartPtr(mcpartHandle, mcpart_i);
      if (fMakeT0Assns){
        util::CreateAssn(*this, evt, *T0col, showerlist[Shower], *Showerassn);
      }
      MCPartShowerassn->addSingle(showerlist[Shower], mcpartPtr, btdata);

    }// Loop over showers
  }


  if (pfparticleListHandle.isValid()){
    //Access pfparticles and hits
    art::FindManyP<recob::Cluster> fmcp(pfparticleListHandle, evt, fPFParticleModuleLabel);
      //art::FindManyP<recob::Hit> fmtht(pfparticleListHandle, evt, fPfparticleModuleLabel);

    size_t NPfparticles = pfparticlelist.size();

    // Now to access MCTruth for each pfparticle...
    for(size_t iPfp=0; iPfp < NPfparticles; ++iPfp) {
      TrueTrackT0 = 0;
      TrackID     = 0;
      TrueTrackID = 0;
      anab::BackTrackerMatchingData btdata;

      std::vector< art::Ptr<recob::Hit> > allHits;
      //Get all hits through associated clusters
      std::vector< art::Ptr<recob::Cluster>> allClusters = fmcp.at(iPfp);
      art::FindManyP<recob::Hit> fmhcl(allClusters, evt, fPFParticleModuleLabel);
      for (size_t iclu = 0; iclu<allClusters.size(); ++iclu){
        std::vector< art::Ptr<recob::Hit>> hits = fmhcl.at(iclu);
        allHits.insert(allHits.end(), hits.begin(), hits.end());
      }

      std::map<int,double> trkide;
      for(size_t h = 0; h < allHits.size(); ++h){
	art::Ptr<recob::Hit> hit = allHits[h];
	std::vector<sim::IDE> ides;
	std::vector<sim::TrackIDE> TrackIDs = bt_serv->HitToTrackIDEs(hit);

	for(size_t e = 0; e < TrackIDs.size(); ++e){
	  trkide[TrackIDs[e].trackID] += TrackIDs[e].energy;
	}
      }
      // Work out which IDE despoited the most charge in the hit if there was more than one.
      double maxe = -1;
      double tote = 0;
      for (std::map<int,double>::iterator ii = trkide.begin(); ii!=trkide.end(); ++ii){
	tote += ii->second;
	if ((ii->second)>maxe){
	  maxe = ii->second;
	  TrackID = ii->first;
	}
      }
      btdata.cleanliness = maxe/tote;

      // Now have trackID, so get PdG code and T0 etc.
      const simb::MCParticle *tmpParticle = pi_serv->TrackIdToParticle_P(TrackID);
      if (!tmpParticle) continue; // Retain this check that the BackTracker can find the right particle
      // Now, loop through the MCParticle's myself to find the correct match
      int mcpart_i(-1);
      for (auto const particle : *mcpartHandle){
        mcpart_i++;
        if (TrackID == particle.TrackId()){
          break;
        }
      }
      const simb::MCParticle particle = mcpartHandle.product()->at(mcpart_i);
      TrueTrackT0 = particle.T();
      TrueTrackID = particle.TrackId();
      TrueTriggerType = 2; // Using MCTruth as trigger, so tigger type is 2.
      //std::cout << "Got particle, PDG = " << particle.PdgCode() << std::endl;

      //std::cout << "Filling T0col with " << TrueTrackT0 << " " << TrueTriggerType << " " << TrueTrackID << " " << (*T0col).size() << std::endl;

      T0col->push_back(anab::T0(TrueTrackT0,
				TrueTriggerType,
				TrueTrackID,
				(*T0col).size()
				));
      //auto diff = particle - firstParticle;
      auto diff = mcpart_i; // check I have a sensible value for this counter
      if (diff >= (int)mcpartHandle->size()){
        std::cout << "Error, the backtracker is doing weird things to your pointers!" << std::endl;
        throw std::exception();
      }
//      art::Ptr<simb::MCParticle> mcpartPtr(mcpartHandle, particle - firstParticle);
      art::Ptr<simb::MCParticle> mcpartPtr(mcpartHandle, mcpart_i);
      //std::cout << "made MCParticle Ptr" << std::endl;
//      const std::vertex< std::pair<double, std::string> > cleanlinessCompleteness;
//      const std::pair<double, double> tmp(0.5, 0.5);
//      MCPartassn->addSingle(tracklist[iPfp], mcpartPtr, tmp);
      if (fMakePFParticleAssns){
        if (fMakeT0Assns){
          util::CreateAssn(*this, evt, *T0col, pfparticlelist[iPfp], *PFParticleassn);
        }
        MCPartPFParticleassn->addSingle(pfparticlelist[iPfp], mcpartPtr, btdata);
      }
      fTree -> Fill();
    } // Loop over tracks
  }

  if (fMakeT0Assns){
    evt.put(std::move(T0col));
    evt.put(std::move(Trackassn));
    evt.put(std::move(Showerassn));
    if (fMakePFParticleAssns){
      evt.put(std::move(PFParticleassn));
    }
  }
  evt.put(std::move(MCPartTrackassn));
  evt.put(std::move(MCPartShowerassn));
  if (fMakePFParticleAssns){
    evt.put(std::move(MCPartPFParticleassn));
  }
  if(fMakeHitAssns)
    evt.put(std::move(MCPartHitassn));
} // Produce

DEFINE_ART_MODULE(t0::MCTruthT0Matching)
