/** \class HLTJetSortedVBFFilter
 *
 * See header file for documentation
 *


 *
 *  \author Jacopo Bernardini
 *
 */


#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Common/interface/RefToBase.h"
#include "HLTrigger/JetMET/interface/HLTJetSortedVBFFilter.h"

#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include<vector>
#include<string>
#include<typeinfo>

using namespace std;
//
// constructors and destructor//
//
template<typename T>
HLTJetSortedVBFFilter<T>::HLTJetSortedVBFFilter(const edm::ParameterSet& iConfig) : HLTFilter(iConfig)
 ,inputJets_   (iConfig.getParameter<edm::InputTag>("inputJets"   ))
 ,inputJetTags_(iConfig.getParameter<edm::InputTag>("inputJetTags"))
 ,mqq_         (iConfig.getParameter<double>       ("Mqq"         ))
 ,detaqq_      (iConfig.getParameter<double>       ("Detaqq"      ))
 ,detabb_      (iConfig.getParameter<double>       ("Detabb"      ))
 ,dphibb_      (iConfig.getParameter<double>       ("Dphibb"      )) 	
 ,ptsqq_       (iConfig.getParameter<double>       ("Ptsumqq"     ))
 ,ptsbb_       (iConfig.getParameter<double>       ("Ptsumbb"     ))
 ,seta_        (iConfig.getParameter<double>       ("Etaq1Etaq2"  ))
 ,value_       (iConfig.getParameter<std::string>  ("value"       ))
 ,triggerType_ (iConfig.getParameter<int>          ("triggerType" ))
{
  m_theJetsToken = consumes<std::vector<T>>(inputJets_);
  m_theJetTagsToken = consumes<reco::JetTagCollection>(inputJetTags_);
}


template<typename T>
HLTJetSortedVBFFilter<T>::~HLTJetSortedVBFFilter()
{ }

template<typename T>
void
HLTJetSortedVBFFilter<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  makeHLTFilterDescription(desc);
  desc.add<edm::InputTag>("inputJets",edm::InputTag("hltJetCollection"));
  desc.add<edm::InputTag>("inputJetTags",edm::InputTag(""));
  desc.add<double>("Mqq",200);
  desc.add<double>("Detaqq",2.5);
  desc.add<double>("Detabb",10.);
  desc.add<double>("Dphibb",10.);
  desc.add<double>("Ptsumqq",0.);
  desc.add<double>("Ptsumbb",0.);
  desc.add<double>("Etaq1Etaq2",40.);
  desc.add<std::string>("value","second");
  desc.add<int>("triggerType",trigger::TriggerJet);
  descriptions.add(string("hlt")+string(typeid(HLTJetSortedVBFFilter<T>).name()),desc);
}

template<typename T> float HLTJetSortedVBFFilter<T>::findCSV(const typename std::vector<T>::const_iterator & jet, const reco::JetTagCollection  & jetTags){
        float minDr = 0.1;
        float tmpCSV = -20 ;
        for (reco::JetTagCollection::const_iterator jetb = jetTags.begin(); (jetb!=jetTags.end()); ++jetb) {
        float tmpDr = reco::deltaR(*jet,*(jetb->first));

        if (tmpDr < minDr) {
                minDr = tmpDr ;
                tmpCSV= jetb->second;
                }

        }
        return tmpCSV;

}
//
// member functions
//

// ------------ method called to produce the data  ------------
template<typename T>
bool
HLTJetSortedVBFFilter<T>::hltFilter(edm::Event& event, const edm::EventSetup& setup,trigger::TriggerFilterObjectWithRefs& filterproduct) const
{

   using namespace std;
   using namespace edm;
   using namespace reco;
   using namespace trigger;

   typedef vector<T> TCollection;
   typedef Ref<TCollection> TRef;

   bool accept(false);
   const unsigned int nMax(4);

   if (saveTags()) filterproduct.addCollectionTag(inputJets_);

   vector<Jpair> sorted(nMax);
   vector<TRef> jetRefs(nMax);

   Handle<TCollection> jets;
   event.getByToken(m_theJetsToken,jets);
   Handle<JetTagCollection> jetTags;

   unsigned int nJet=0;
   double value(0.0);

   Particle::LorentzVector b1,b2,q1,q2;

   if (inputJetTags_.encode()=="") {
     if (jets->size()<nMax) return false;
     for (typename TCollection::const_iterator jet=jets->begin(); (jet!=jets->end()&& nJet<nMax); ++jet) {
       if (value_=="Pt") {
	 value=jet->pt();
       } else if (value_=="Eta") {
	 value=jet->eta();
       } else if (value_=="Phi") {
	 value=jet->phi();
       } else {
	 value = 0.0;
       }
       sorted[nJet] = make_pair(value,nJet);
       ++nJet;
     }
     sort(sorted.begin(),sorted.end(),comparator);
     for (unsigned int i=0; i<nMax; ++i) {
       jetRefs[i]=TRef(jets,sorted[i].second);
     }
     q1 = jetRefs[3]->p4();
     b1 = jetRefs[2]->p4();
     b2 = jetRefs[1]->p4();
     q2 = jetRefs[0]->p4();
   } else {
     event.getByToken(m_theJetTagsToken,jetTags);


     if (jetTags->size()<nMax) return false;
     for (typename TCollection::const_iterator jet=jets->begin(); (jet!=jets->end()&& nJet<nMax); ++jet) {

       if (value_=="second") {
	 value = findCSV(jet, *jetTags);
       } else {
	 value = 0.0;
       }
       sorted[nJet] = make_pair(value,nJet);
       ++nJet;
     }
     sort(sorted.begin(),sorted.end(),comparator);
     for (unsigned int i=0; i<nMax; ++i) {
       jetRefs[i]= TRef(jets,(*jetTags)[sorted[i].second].first.key());
     }
     b1 = jetRefs[3]->p4();
     b2 = jetRefs[2]->p4();
     q1 = jetRefs[1]->p4();
     q2 = jetRefs[0]->p4();
   }

   double mqq_bs     = (q1+q2).M();
   double deltaetaqq = std::abs(q1.Eta()-q2.Eta());
   double deltaetabb = std::abs(b1.Eta()-b2.Eta());
   double deltaphibb = std::abs(reco::deltaPhi(b1.Phi(),b2.Phi()));
   double ptsqq_bs   = (q1+q2).Pt();
   double ptsbb_bs   = (b1+b2).Pt();
   double signeta    = q1.Eta()*q2.Eta();

   if (
	(mqq_bs     > mqq_    ) &&
	(deltaetaqq > detaqq_ ) &&
	(deltaetabb < detabb_ ) &&
	(deltaphibb < dphibb_ ) &&
	(ptsqq_bs   > ptsqq_  ) &&
	(ptsbb_bs   > ptsbb_  ) &&
	(signeta    < seta_   )
	) {
     accept=true;
     for (unsigned int i=0; i<nMax; ++i) {
       filterproduct.addObject(triggerType_,jetRefs[i]);
     }
   }

   return accept;
}
