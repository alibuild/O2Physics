// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file HFCorrelatorDplusHadrons.cxx
 //Service-Oriented Architecture (SOA)
/// \author Shyam Kumar <shyam.kumar@cern.ch>
#include "Framework/AnalysisTask.h"
#include "Framework/HistogramRegistry.h"
#include "PWGHF/DataModel/HFSecondaryVertex.h"
#include "PWGHF/DataModel/HFCandidateSelectionTables.h"
#include "Common/Core/TrackSelection.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Framework/runDataProcessing.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::aod::hf_cand_prong3;
using namespace o2::aod::hf_correlation_dplushadron;
using namespace o2::analysis::hf_cuts_dplus_topikpi;
using namespace o2::constants::math;

/// Returns deltaPhi value in range [-pi/2., 3.*pi/2], typically used for correlation studies

double getDeltaPhi(double phiD, double phiHadron)
{
  return RecoDecay::constrainAngle(phiHadron - phiD, -o2::constants::math::PI / 2.);
}

/// definition of variables for Dplus hadron pairs vs eta acceptance studies (hDplusHadronVsEtaCut, in data-like, MC-reco and MC-kine tasks)
const double maxEtaCut = 5.;
const double ptThresholdForMaxEtaCut = 10.;
const double incrementEtaCut = 0.1;
const double incrementPtThreshold = 0.5;
const double epsilon = 1E-5;
// npTBinsMassAndEfficiency = 12 for D+ and 25 for D0
const int npTBinsMassAndEfficiency = o2::analysis::hf_cuts_dplus_topikpi::npTBins;
const double efficiencyDmesonDefault[npTBinsMassAndEfficiency] = {};
auto efficiencyDmeson_v = std::vector<double>{efficiencyDmesonDefault, efficiencyDmesonDefault + npTBinsMassAndEfficiency};

// histogram binning definition 
const int massAxisBins = 350;
const double massAxisMin = 1.7;
const double massAxisMax = 2.05;
const int phiAxisBins = 32;
const double phiAxisMin = 0.;
const double phiAxisMax = 2. * o2::constants::math::PI;
const int yAxisBins = 100;
const double yAxisMin = -5.;
const double yAxisMax = 5.;
const int ptDAxisBins = 180;
const double ptDAxisMin = 0.;
const double ptDAxisMax = 36.;

using MCParticlesPlus3Prong = soa::Join<aod::McParticles, aod::HfCandProng3MCGen>;

/// Dplus-Hadron correlation pair builder - for real data and data-like analysis (i.e. reco-level w/o matching request via MC truth)
struct HfCorrelatorDplusHadrons {
  Produces<aod::DplusHadronPair> entryDplusHadronPair;
  Produces<aod::DplusHadronRecoInfo> entryDplusHadronRecoInfo;

  HistogramRegistry registry{
    "registry",
    //NOTE: use hMassD0 for trigger normalisation (S*0.955), and hMass2DCorrelationPairs (in final task) for 2D-sideband-subtraction purposes
    {
     {"hPtCand", "Dplus,Hadron candidates;candidate #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng0", "Dplus,Hadron candidates;prong 0 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng1", "Dplus,Hadron candidates;prong 1 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng2", "Dplus,Hadron candidates;prong 2 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hSelectionStatus", "Dplus,Hadron candidates;selection status;entries", {HistType::kTH1F, {{4, -0.5, 3.5}}}},
     {"hEta", "Dplus,Hadron candidates;candidate #it{#eta};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hPhi", "Dplus,Hadron candidates;candidate #it{#varphi};entries", {HistType::kTH1F, {{phiAxisBins, phiAxisMin, phiAxisMax}}}},
     {"hY", "Dplus,Hadron candidates;candidate #it{#y};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hPtCandMCRec", "Dplus,Hadron candidates - MC reco;candidate #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng0MCRec", "Dplus,Hadron candidates - MC reco;prong 0 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng1MCRec", "Dplus,Hadron candidates - MC reco;prong 1 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hPtProng2MCRec", "Dplus,Hadron candidates - MC reco;prong 2 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hSelectionStatusMCRec", "Dplus,Hadron candidates - MC reco;selection status;entries", {HistType::kTH1F, {{4, -0.5, 3.5}}}},
     {"hEtaMCRec", "Dplus,Hadron candidates - MC reco;candidate #it{#eta};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hPhiMCRec", "Dplus,Hadron candidates - MC reco;candidate #it{#varphi};entries", {HistType::kTH1F, {{phiAxisBins, phiAxisMin, phiAxisMax}}}},
     {"hYMCRec", "Dplus,Hadron candidates - MC reco;candidate #it{#y};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hMCEvtCount", "Event counter - MC gen;;entries", {HistType::kTH1F, {{1, -0.5, 0.5}}}},
     {"hPtCandMCGen", "Dplus,Hadron particles - MC gen;particle #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{ptDAxisBins, ptDAxisMin, ptDAxisMax}}}},
     {"hEtaMCGen", "Dplus,Hadron particles - MC gen;particle #it{#eta};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hPhiMCGen", "Dplus,Hadron particles - MC gen;particle #it{#varphi};entries", {HistType::kTH1F, {{phiAxisBins, phiAxisMin, phiAxisMax}}}},
     {"hYMCGen", "Dplus,Hadron candidates - MC gen;candidate #it{#y};entries", {HistType::kTH1F, {{yAxisBins, yAxisMin, yAxisMax}}}},
     {"hcountDplusHadronPerEvent", "Dplus,Hadron particles - MC gen;Number per event;entries", {HistType::kTH1F, {{20, 0., 20.}}}},
     {"hDplusDaughtersEtaCut", "Dplus daughters vs #eta cut on D daughters;#eta_{max};entries", {HistType::kTH2F, {{(int)(maxEtaCut / incrementEtaCut), 0., maxEtaCut}, {(int)(ptThresholdForMaxEtaCut / incrementPtThreshold), 0., ptThresholdForMaxEtaCut}}}},
     {"hMultiplicityPreSelection", "multiplicity prior to selection;multiplicity;entries", {HistType::kTH1F, {{10000, 0., 10000.}}}},
     {"hMultiplicity", "multiplicity;multiplicity;entries", {HistType::kTH1F, {{10000, 0., 10000.}}}},
     {"hDplusHadronVsEtaCut", "Dplus,Hadron pairs vs #eta cut;#eta_{max};entries", {HistType::kTH2F, {{(int)(maxEtaCut / incrementEtaCut), 0., maxEtaCut}, {(int)(ptThresholdForMaxEtaCut / incrementPtThreshold), 0., ptThresholdForMaxEtaCut}}}}
   }
 };

  Configurable<int> selectionFlagDplus{"selectionFlagDplus", 1, "Selection Flag for Dplus"};
  Configurable<double> cutYCandMax{"cutYCandMax", -1., "max. cand. rapidity"};
  Configurable<double> cutPtCandMin{"cutPtCandMin", -1., "min. cand. pT"};
  Configurable<double> cutPtCandMax{"cutPtCandMax", -1., "max. cand. pT"};
  Configurable<std::vector<double>> bins{"ptBinsForMassAndEfficiency", std::vector<double>{o2::analysis::hf_cuts_dplus_topikpi::pTBins_v}, "pT bin limits for candidate mass plots and efficiency"};
  Configurable<std::vector<double>> efficiencyDmeson{"efficiencyDmeson", std::vector<double>{efficiencyDmeson_v}, "Efficiency values for Dplus meson"};
  Configurable<int> flagApplyEfficiency{"efficiencyFlagD", 1, "Flag for applying D-meson efficiency weights"};
  Configurable<double> multMin{"multMin", 0., "minimum multiplicity accepted"};
  Configurable<double> multMax{"multMax", 10000., "maximum multiplicity accepted"};

  void init(o2::framework::InitContext&)
  {
    auto vbins = (std::vector<double>)bins;
    registry.add("hMassDplus_2D", "Dplus candidates;inv. mass (K^{-}#pi^{+}#pi^{+}) (GeV/#it{c}^{2});entries", {HistType::kTH2F, {{massAxisBins, massAxisMin, massAxisMax}, {vbins, "#it{p}_{T} (GeV/#it{c})"}}});
    registry.add("hMassDplusData", "Dplus candidates;inv. mass (K^{-}#pi^{+}#pi^{+}) (GeV/#it{c}^{2});entries", {HistType::kTH1F, {{massAxisBins, massAxisMin, massAxisMax}}});
    registry.add("hMassDplusMCRec", "Dplus candidates;inv. mass (K^{-}#pi^{+}#pi^{+}) (GeV/#it{c}^{2});entries", {HistType::kTH1F, {{massAxisBins, massAxisMin, massAxisMax}}});
    registry.add("hMassDplusMCRecSig", "Dplus signal candidates - MC reco;inv. mass (#pi K) (GeV/#it{c}^{2});entries", {HistType::kTH2F, {{massAxisBins, massAxisMin, massAxisMax}, {vbins, "#it{p}_{T} (GeV/#it{c})"}}});
    registry.add("hMassDplusMCRecBkg", "Dplus background candidates - MC reco;inv. mass (#pi K) (GeV/#it{c}^{2});entries", {HistType::kTH2F, {{massAxisBins, massAxisMin, massAxisMax}, {vbins, "#it{p}_{T} (GeV/#it{c})"}}});
    registry.add("hcountDplustriggersMCGen", "Dplus trigger particles - MC gen;;N of trigger Dplus", {HistType::kTH2F, {{1, -0.5, 0.5}, {vbins, "#it{p}_{T} (GeV/#it{c})"}}});  
  }

  // Select only DPlus 
  Filter filterSelectCandidates = (aod::hf_selcandidate_dplus::isSelDplusToPiKPi >= selectionFlagDplus);

  void processData(aod::Collision const& collision, soa::Join<aod::Tracks, aod::TracksDCA>& tracks, soa::Filtered<soa::Join<aod::HfCandProng3, aod::HFSelDplusToPiKPiCandidate>> const& candidates)
  {
    int nTracks = 0; 
    if (collision.numContrib() > 1) {
      for (const auto& track : tracks) {
        if (track.eta() < -4.0 || track.eta() > 4.0) {
          continue;
        }
        if (abs(track.dcaXY()) > 0.0025 || abs(track.dcaZ()) > 0.0025) { // change for Dplus
          continue;
        }
        nTracks++;
      }
    }

    registry.fill(HIST("hMultiplicityPreSelection"), nTracks);
    if (nTracks < multMin || nTracks > multMax) {
      return;
    }
    registry.fill(HIST("hMultiplicity"), nTracks);

    for (auto& candidate1 : candidates) {
      if (cutYCandMax >= 0. && std::abs(YDPlus(candidate1)) > cutYCandMax) {
        continue;
      }
      if (cutPtCandMin >= 0. && candidate1.pt() < cutPtCandMin) {
        continue;
      }
      if (candidate1.pt() > cutPtCandMax) {
        continue;
      }
      //check decay channel flag for candidate1
      if (!(candidate1.hfflag() & 1 << DecayType::DPlusToPiKPi)) { // check Left Shift and Right Shift Operators C++
        continue;
      }
      double efficiencyWeight = 1.;
      if (flagApplyEfficiency) {
        efficiencyWeight = 1. / efficiencyDmeson->at(o2::analysis::findBin(bins, candidate1.pt()));
      }
      //fill invariant mass plots and generic info from all Dplus candidates
      if (candidate1.isSelDplusToPiKPi() >= selectionFlagDplus) {
        registry.fill(HIST("hMassDplus_2D"), InvMassDPlus(candidate1), candidate1.pt(), efficiencyWeight);
        registry.fill(HIST("hMassDplusData"), InvMassDPlus(candidate1), efficiencyWeight);
      }
      registry.fill(HIST("hPtCand"), candidate1.pt());
      registry.fill(HIST("hPtProng0"), candidate1.ptProng0());
      registry.fill(HIST("hPtProng1"), candidate1.ptProng1());
      registry.fill(HIST("hPtProng2"), candidate1.ptProng2());
      registry.fill(HIST("hEta"), candidate1.eta());
      registry.fill(HIST("hPhi"), candidate1.phi());
      registry.fill(HIST("hY"), YDPlus(candidate1));
      registry.fill(HIST("hSelectionStatus"), candidate1.isSelDplusToPiKPi());

      //Dplus-Hadron correlation dedicated section
      //if the candidate is a Dplus, search for Hadrons and evaluate correlations
      if (candidate1.isSelDplusToPiKPi() < selectionFlagDplus) {
        continue;
      }
       for (const auto& track : tracks) {
        //Removing Dplus daughters by checking track indices
        if ((candidate1.index0Id() == track.mRowIndex) || (candidate1.index1Id() == track.mRowIndex) || (candidate1.index2Id() == track.mRowIndex)) {
          continue;
        }
        if (abs(track.dcaXY()) >= 1.0 || abs(track.dcaZ()) >= 1.0) continue; // Remove secondary tracks
        entryDplusHadronPair(getDeltaPhi(track.phi(), candidate1.phi()),
                         track.eta() - candidate1.eta(),
                         candidate1.pt(),
                         track.pt());
        entryDplusHadronRecoInfo(InvMassDPlus(candidate1),0);
        double etaCut = 0.;
        double ptCut = 0.;
        do { //fill pairs vs etaCut plot
          ptCut = 0.;
          etaCut += incrementEtaCut;
          do { //fill pairs vs etaCut plot
            if (std::abs(candidate1.eta()) < etaCut && std::abs(track.eta()) < etaCut && candidate1.pt() > ptCut && track.pt() > ptCut) {
              registry.fill(HIST("hDplusHadronVsEtaCut"), etaCut - epsilon, ptCut + epsilon);
            }
            ptCut += incrementPtThreshold;
          } while (ptCut < ptThresholdForMaxEtaCut - epsilon);
        } while (etaCut < maxEtaCut - epsilon);
        //note: candidates selected as both D0 and D0bar are used, and considered in both situation (but not auto-correlated): reflections could play a relevant role.
        //another, more restrictive, option, could be to consider only candidates selected with a single option (D0 xor D0bar)

      } // Hadron Tracks loop

    } //end outer loop
  }

PROCESS_SWITCH(HfCorrelatorDplusHadrons, processData, "Process data", false);

/// Dplus-Hadron correlation pair builder - for MC reco-level analysis (candidates matched to true signal only, but also the various bkg sources are studied)
void processMcRec(aod::Collision const& collision, soa::Join<aod::Tracks, aod::TracksDCA>& tracks, soa::Filtered<soa::Join<aod::HfCandProng3, aod::HFSelDplusToPiKPiCandidate, aod::HfCandProng3MCRec>> const& candidates)
  {
    int nTracks = 0;
    if (collision.numContrib() > 1) {
      for (const auto& track : tracks) {
        if (track.eta() < -4.0 || track.eta() > 4.0) {
          continue;
        }
        if (abs(track.dcaXY()) > 0.0025 || abs(track.dcaZ()) > 0.0025) {
          continue;
        }
        nTracks++;
      }
    }
    //printf("cuts on Y max %f \n",(double)cutYCandMax);
    registry.fill(HIST("hMultiplicityPreSelection"), nTracks);
    if (nTracks < multMin || nTracks > multMax) {
      return;
    }
    registry.fill(HIST("hMultiplicity"), nTracks);

    //MC reco level
     bool flagDplusSignal = false;
    for (auto& candidate1 : candidates) {
      //check decay channel flag for candidate1
      if (!(candidate1.hfflag() & 1 << DecayType::DPlusToPiKPi)) {
        continue;
      }
      if (cutYCandMax >= 0. && std::abs(YDPlus(candidate1)) > cutYCandMax) {
        continue;
      }
      if (cutPtCandMin >= 0. && candidate1.pt() < cutPtCandMin) {
        continue;
      }
      if (candidate1.pt() >= cutPtCandMax) {
        continue;
      }
      double efficiencyWeight = 1.;
      if (flagApplyEfficiency) {
        efficiencyWeight = 1. / efficiencyDmeson->at(o2::analysis::findBin(bins, candidate1.pt()));
      }

      if (std::abs(candidate1.flagMCMatchRec()) == 1 << DecayType::DPlusToPiKPi) {
        //fill per-candidate distributions from Dplus true candidates
        registry.fill(HIST("hPtCandMCRec"), candidate1.pt());
        registry.fill(HIST("hPtProng0MCRec"), candidate1.ptProng0());
        registry.fill(HIST("hPtProng1MCRec"), candidate1.ptProng1());
        registry.fill(HIST("hPtProng2MCRec"), candidate1.ptProng2());
        registry.fill(HIST("hEtaMCRec"), candidate1.eta());
        registry.fill(HIST("hPhiMCRec"), candidate1.phi());
        registry.fill(HIST("hYMCRec"), YDPlus(candidate1));
        registry.fill(HIST("hSelectionStatusMCRec"), candidate1.isSelDplusToPiKPi());
      }
      //fill invariant mass plots from Dplus signal and background candidates
      if (candidate1.isSelDplusToPiKPi() >= selectionFlagDplus) {                  //only reco as Dplus
        registry.fill(HIST("hMassDplusMCRec"), InvMassDPlus(candidate1), efficiencyWeight);
        if (candidate1.flagMCMatchRec() == 1 << DecayType::DPlusToPiKPi) { //also matched as D0
          registry.fill(HIST("hMassDplusMCRecSig"), InvMassDPlus(candidate1), candidate1.pt(), efficiencyWeight);
        } else {
          registry.fill(HIST("hMassDplusMCRecBkg"), InvMassDPlus(candidate1), candidate1.pt(), efficiencyWeight);
        }
      }

      //Dplus-Hadron correlation dedicated section
      //if the candidate is selected as Dplus, search for Hadron and evaluate correlations
      if (candidate1.isSelDplusToPiKPi() < selectionFlagDplus) { //discard candidates not selected as Dplus in outer loop
        continue;
      }
      flagDplusSignal = candidate1.flagMCMatchRec() == 1 << DecayType::DPlusToPiKPi;   //flagDplusSignal 'true' if candidate1 matched to Dplus (particle)
      for (const auto& track : tracks) {

        //Removing Dplus daughters by checking track indices
        if ((candidate1.index0Id()== track.mRowIndex) || (candidate1.index1Id() == track.mRowIndex) || (candidate1.index2Id()== track.mRowIndex)) {
          continue;
        }
        if (abs(track.dcaXY()) >= 1.0 || abs(track.dcaZ()) >= 1.0) continue; // Remove secondary tracks
        entryDplusHadronPair(getDeltaPhi(track.phi(), candidate1.phi()),
                         track.eta() - candidate1.eta(),
                         candidate1.pt(),
                         track.pt());
        entryDplusHadronRecoInfo(InvMassDPlus(candidate1), (int)flagDplusSignal);
        double etaCut = 0.;
        double ptCut = 0.;
        do { //fill pairs vs etaCut plot
          ptCut = 0.;
          etaCut += incrementEtaCut;
          do { //fill pairs vs etaCut plot
            if (std::abs(candidate1.eta()) < etaCut && std::abs(track.eta()) < etaCut && candidate1.pt() > ptCut && track.pt() > ptCut) {
              registry.fill(HIST("hDplusHadronVsEtaCut"), etaCut - epsilon, ptCut + epsilon);
            }
            ptCut += incrementPtThreshold;
          } while (ptCut < ptThresholdForMaxEtaCut - epsilon);
        } while (etaCut < maxEtaCut - epsilon);
      } // end inner loop (Tracks)

    } //end outer loop
  }

 PROCESS_SWITCH(HfCorrelatorDplusHadrons, processMcRec, "Process MC Reco mode", true);
/// Dplus-Hadron correlation pair builder - for MC gen-level analysis (no filter/selection, only true signal)
  void processMcGen(aod::McCollision const& mccollision, MCParticlesPlus3Prong const& particlesMC)
  {
    int counterDplusHadron = 0;
    registry.fill(HIST("hMCEvtCount"), 0);
    //MC gen level
    for (auto& particle1 : particlesMC) {
      //check if the particle is Dplus  (for general plot filling and selection, so both cases are fine) - NOTE: decay channel is not probed!
      if (std::abs(particle1.pdgCode()) != pdg::Code::kDPlus) {
        continue;
      }
      double yD = RecoDecay::Y(array{particle1.px(), particle1.py(), particle1.pz()}, RecoDecay::getMassPDG(particle1.pdgCode()));
      if (cutYCandMax >= 0. && std::abs(yD) > cutYCandMax) {
        continue;
      }
      if (cutPtCandMin >= 0. && particle1.pt() < cutPtCandMin) {
        continue;
      }
      registry.fill(HIST("hPtCandMCGen"), particle1.pt());
      registry.fill(HIST("hEtaMCGen"), particle1.eta());
      registry.fill(HIST("hPhiMCGen"), particle1.phi());
      registry.fill(HIST("hYMCGen"), yD);
      counterDplusHadron++;
      //Dplus Hadron correlation dedicated section
      //if it's a Dplus particle, search for Hadron and evaluate correlations
      if (particle1.pdgCode() != pdg::Code::kDPlus) { //just checking the particle PDG, not the decay channel (differently from Reco: you have a BR factor btw such levels!)
        continue;
      }
      registry.fill(HIST("hcountDplustriggersMCGen"), 0, particle1.pt()); //to count trigger Dplus for normalisation)
      for (auto& particle2 : particlesMC) {
        if ((std::abs(particle2.pdgCode()) != 11) && (std::abs(particle2.pdgCode()) != 13) && (std::abs(particle2.pdgCode()) != 211) && (std::abs(particle2.pdgCode()) != 321) && (std::abs(particle2.pdgCode()) != 2212)) { //check that inner particle is Dplus
          continue;
        }
        entryDplusHadronPair(getDeltaPhi(particle2.phi(), particle1.phi()),
                         particle2.eta() - particle1.eta(),
                         particle1.pt(),
                         particle2.pt());
        double etaCut = 0.;
        double ptCut = 0.;

        //fill pairs vs etaCut plot
        bool rightDecayChannels = false;
        if ((std::abs(particle1.flagMCMatchGen()) == 1 << DecayType::DPlusToPiKPi)) {
          rightDecayChannels = true;
        }
        do {
          ptCut = 0.;
          etaCut += incrementEtaCut;
          do {                                                                                                                                  //fill pairs vs etaCut plot
            if (std::abs(particle1.eta()) < etaCut && std::abs(particle2.eta()) < etaCut && particle1.pt() > ptCut && particle2.pt() > ptCut) { //fill with Dplus Hadron in acceptance checks
              registry.fill(HIST("hDplusHadronVsEtaCut"), etaCut - epsilon, ptCut + epsilon);
            }
            if (rightDecayChannels) { //fill with Dplus daughter particles acceptance checks
              bool candidate1DauInAcc = true;
              for (auto& dau : particle1.daughters_as<MCParticlesPlus3Prong>()) {
                if (std::abs(dau.eta()) > etaCut) {
                  candidate1DauInAcc = false;
                  break;
                }
              }
              if (candidate1DauInAcc && particle1.pt() > ptCut && particle1.pt() > ptCut) {
              registry.fill(HIST("hDplusDaughtersEtaCut"), etaCut - epsilon, ptCut + epsilon);
              }
              }
            ptCut += incrementPtThreshold;
          } while (ptCut < ptThresholdForMaxEtaCut - epsilon);
        } while (etaCut < maxEtaCut - epsilon);
      } //end inner loop
    }   //end outer loop
    registry.fill(HIST("hcountDplusHadronPerEvent"), counterDplusHadron);
  }
   PROCESS_SWITCH(HfCorrelatorDplusHadrons, processMcGen, "Process MC Gen mode", false);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<HfCorrelatorDplusHadrons>(cfgc)};
}
