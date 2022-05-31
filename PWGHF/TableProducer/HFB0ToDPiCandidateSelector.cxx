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

/// \file HFB0ToDPiCandidateSelector.cxx
/// \brief B0 → D- π+ candidate selector
///
/// \author Panos Christakoglou <panos.christakoglou@cern.ch>, Nikhef

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "PWGHF/Core/HFSelectorCuts.h"
#include "PWGHF/DataModel/HFSecondaryVertex.h"
#include "PWGHF/DataModel/HFCandidateSelectionTables.h"
#include "PWGHF/Core/HFSelectorCuts.h"

using namespace o2;
using namespace o2::aod;
using namespace o2::framework;
using namespace o2::aod::hf_cand_b0;	// from HFSecondaryVertex.h
using namespace o2::analysis;
using namespace o2::aod::hf_cand_prong2;
using namespace o2::analysis::hf_cuts_b0_todpi;	// from HFSelectorCuts.h

struct HfB0ToDPiCandidateSelector {
  Produces<aod::HFSelB0ToDPiCandidate> hfSelB0ToDPiCandidate; // table defined in HFCandidateSelectionTables.h

  Configurable<double> pTCandMin{"pTCandMin", 0., "Lower bound of candidate pT"};
  Configurable<double> pTCandMax{"pTCandMax", 50., "Upper bound of candidate pT"};

  //Track quality
  Configurable<double> TPCNClsFindablePIDCut{"TPCNClsFindablePIDCut", 70., "Lower bound of TPC findable clusters for good PID"};

  //TPC PID
  Configurable<double> pidTPCMinpT{"pidTPCMinpT", 0.15, "Lower bound of track pT for TPC PID"};
  Configurable<double> pidTPCMaxpT{"pidTPCMaxpT", 10., "Upper bound of track pT for TPC PID"};
  Configurable<double> nSigmaTPC{"nSigmaTPC", 5., "Nsigma cut on TPC only"};
  Configurable<double> nSigmaTPCCombined{"nSigmaTPCCombined", 5., "Nsigma cut on TPC combined with TOF"};

  //TOF PID
  Configurable<double> pidTOFMinpT{"pidTOFMinpT", 0.15, "Lower bound of track pT for TOF PID"};
  Configurable<double> pidTOFMaxpT{"pidTOFMaxpT", 10., "Upper bound of track pT for TOF PID"};
  Configurable<double> nSigmaTOF{"nSigmaTOF", 5., "Nsigma cut on TOF only"};
  Configurable<double> nSigmaTOFCombined{"nSigmaTOFCombined", 5., "Nsigma cut on TOF combined with TPC"};

  Configurable<std::vector<double>> pTBins{"pTBins", std::vector<double>{hf_cuts_b0_todpi::pTBins_v}, "pT bin limits"};
  Configurable<LabeledArray<double>> cuts{"B0_to_dpi_cuts", {hf_cuts_b0_todpi::cuts[0], npTBins, nCutVars, pTBinLabels, cutVarLabels}, "B0 candidate selection per pT bin"};
  Configurable<int> selectionFlagD{"selectionFlagD", 1, "Selection Flag for D^{#minus}"};

  // Apply topological cuts as defined in HFSelectorCuts.h; return true if candidate passes all cuts
  template <typename T1, typename T2, typename T3>
  bool selectionTopol(const T1& hfCandB0, const T2& hfCandD, const T3& trackPi)
  {
    auto candpT = hfCandB0.pt();
    int pTBin = findBin(pTBins, candpT);
    if (pTBin == -1) {
      // Printf("B0 topol selection failed at getpTBin");
      return false;
    }

    // check that the candidate pT is within the analysis range
    if (candpT < pTCandMin || candpT >= pTCandMax) {
      return false;
    }

    //B0 mass cut
    if (std::abs(InvMassB0(hfCandB0) - RecoDecay::getMassPDG(pdg::Code::kB0)) > cuts->get(pTBin, "m")) {
      //Printf("B0 topol selection failed at mass diff check");
      return false;
    }

    //pion pt
    if (trackPi.pt() < cuts->get(pTBin, "pT Pi")) {
      return false;
    }

    //D- pt
    if (hfCandD.pt() < cuts->get(pTBin, "pT D^{#minus}")) {
      return false;
    }

    //Lc mass
    //if (trackPi.sign() < 0) {
    //if (std::abs(InvMassLcpKpi(hfCandLc) - RecoDecay::getMassPDG(pdg::Code::kLambdaCPlus)) > cuts->get(pTBin, "DeltaMLc")) {
    //return false;
    //}
    //}

    //Lb Decay length
    if (hfCandB0.decayLength() < cuts->get(pTBin, "Lb decLen")) {
      return false;
    }

    //Lb Decay length XY
    if (hfCandB0.decayLengthXY() < cuts->get(pTBin, "Lb decLenXY")) {
      return false;
    }

    //Lb chi2PCA cut
    if (hfCandB0.chi2PCA() > cuts->get(pTBin, "Chi2PCA")) {
      //Printf("Lb selection failed at chi2PCA");
      return false;
    }

    //Lb CPA cut
    if (hfCandB0.cpa() < cuts->get(pTBin, "CPA")) {
      return false;
    }

    //d0 of pi
    if (std::abs(hfCandB0.impactParameter1()) < cuts->get(pTBin, "d0 Pi")) {
      return false;
    }

    //d0 of D
    if (std::abs(hfCandB0.impactParameter0()) < cuts->get(pTBin, "d0 D^{#minus}")) {
      return false;
    }

    return true;
  }

  void process(aod::HfCandB0 const& hfCandB0s, soa::Join<aod::HfCandProng3, aod::HFSelDplusToPiKPiCandidate>, aod::BigTracksPID const&)
  {
    for (auto& hfCandB0 : hfCandB0s) { //looping over Lb candidates

      int statusB0 = 0;

      // check if flagged as Λb --> Λc+ π-
      if (!(hfCandB0.hfflag() & 1 << hf_cand_b0::DecayType::B0ToDPi)) {
        hfSelB0ToDPiCandidate(statusB0);
        //Printf("B0 candidate selection failed at hfflag check");
        continue;
      }

      // D is always index0 and pi is index1 by default
      //auto candD = hfCandD.index0();
      auto candD = hfCandB0.index0_as<soa::Join<aod::HfCandProng3, aod::HFSelDplusToPiKPiCandidate>>();
      auto trackPi = hfCandB0.index1_as<aod::BigTracksPID>();

      //topological cuts
      if (!selectionTopol(hfCandB0, candD, trackPi)) {
        hfSelB0ToDPiCandidate(statusB0);
        // Printf("B0 candidate selection failed at selection topology");
        continue;
      }

      hfSelB0ToDPiCandidate(1);
      //Printf("B0 candidate selection successful, candidate should be selected");
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow{};
  workflow.push_back(adaptAnalysisTask<HfB0ToDPiCandidateSelector>(cfgc, TaskName{"hf-b0-todpi-candidate-selector"}));
  return workflow;
}
