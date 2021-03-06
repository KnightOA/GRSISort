/*
 * Magical Nonlinear finder and TSpline Generator
 *
 * Created by Kurtis Raymond (kraymond@sfu.ca)
 *
 * This script should operate on a TTree that has had its singles
 * channels already gain matched.
 *
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <math.h> // round, floor, ceil, trunc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

#include "TApplication.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TF1.h"
#include "TFile.h"
#include "TGainMatch.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "THStack.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TMath.h"
#include "TPPG.h"
#include "TPad.h"
#include "TPeak.h"
#include "TROOT.h"
#include "TScaler.h"
#include "TSpectrum.h"
#include "TSpline.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"

#ifndef __CINT__
#include "TGriffin.h"
#include "TSceptar.h"
#endif

//=== Global Variables ===//
// 1 for fragment tree
// 0 for analysis tree
const bool gIsFragmentFile = 0;

// Input calibration peaks for finding nonlinearities
/*
const std::vector<double_t> gPeaks = {121.7817, 244.6974, 964.057,
                                      1085.837, 1112.076};

const std::vector<double_t> gWidths = {16, 20, 20, 13, 13};
*/

const std::vector<double_t> gPeaks = {315.42, 769.31, 1864.89,
                                      2118.26, 3275.16};

const std::vector<double_t> gWidths = {20, 20, 20, 20, 20};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf(" Usage: %s <fragment or analysis tree file> ).\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    TFile *pFile = new TFile(argv[1], "UPDATE");

    if (!pFile->IsOpen()) {
        printf("Failed to open file '%s'\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if( gPeaks.size() != gWidths.size() ) {
        printf("Number of peaks and widths are different!\n");
        exit(EXIT_FAILURE);
    }

    TTree *pTree = nullptr;

    if (gIsFragmentFile)
        pTree = (TTree *)pFile->Get("FragmentTree");
    else
        pTree = (TTree *)pFile->Get("AnalysisTree");

    TChannel::ReadCalFromTree(pTree);

    if (pTree == nullptr) {
        printf("Failed to find fragment or analysis tree in file '%s'.\n",
               argv[1]);
        exit(EXIT_FAILURE);
    }

    // TGRSIRunInfo* runInfo = (TGRSIRunInfo*) pFile->Get("TGRSIRunInfo");
    // int runNum = runInfo->RunNumber();

    // Setup TGriffin
    TGriffin *pGriff = nullptr;
    pTree->SetBranchAddress("TGriffin", &pGriff);

    printf("Generating empty matrix");
    // Load energy matrix
    TH2D *mat_en = new TH2D("mat_en", "", 64, 0, 64, 5000, 0, 5000);

    printf("Filling energy matrix");
    // Load in Energy data
    if (gIsFragmentFile)
        pTree->Project("mat_en", "TFragment.GetEnergy():"
                                 "TFragment.GetChannelNumber()");
    else
        pTree->Project("mat_en",
                       "TGriffin.fGriffinLowGainHits.GetEnergy():"
                       "TGriffin.fGriffinLowGainHits.GetChannel().fNumber");

    // Make a list that will store all the energy differences
    TList* NonLinearityList = new TList();
    TList* NonLinearityTGraph = new TList();
    /*
    TMultiGraph* ResidualCryst[15];
    for (int i = 0; i<16 ; i++)
        ResidualCryst[i] = new TMultiGraph();
    */

    int nPeaks = gPeaks.size();

    for (int i = 0; i < 64; i++) {
        printf("Starting new channel %d:\n", i);

        // Project the project the histogram for the current channel
        TH1D *h_en = mat_en->ProjectionY(Form("h_%.2i", i), i + 1, i + 1);

        std::vector<double_t> EngDiff = {};
        std::vector<double_t> EngX = {};

        // Fit all the peaks in our calibration and collect their centroids
        for (int k = 0; k < nPeaks; k++) {
            double_t CalPeak, DataPeak, CalWidth;
            CalPeak = gPeaks[k];
            CalWidth = gWidths[k];
            printf("Fitting peak %g.\n", gPeaks[k]);
            // We use TSpectrum::Search() to grab all the peaks. Output is
            // ordered from the most intense peak to the least.
            TSpectrum s;
            h_en->GetXaxis()->SetRangeUser(CalPeak - CalWidth,
                                           CalPeak + CalWidth);
            s.Search(h_en, 2, "", 0.25);    // Hist, Sigma, Opt, Threshold
            DataPeak = s.GetPositionX()[0]; // Grab most intense peak
            if ( DataPeak < 1 ) // Errors commonly fall under this condition
                continue;
            printf("Roughly at %g ", DataPeak);
            h_en->GetXaxis()->UnZoom();

            // Fit the peak
            TPeak *CurPeak =
                new TPeak(DataPeak, DataPeak - CalWidth, DataPeak + CalWidth);
            CurPeak->Fit(h_en, "MQ+"); // Quiet Flag
            DataPeak = CurPeak->GetCentroid();

            // Report the peak
            printf("... found at %g, ", DataPeak);
            EngDiff.push_back(CalPeak - DataPeak);
            printf(" difference of %g\n", EngDiff.back());
            EngX.push_back(gPeaks[k]);

            // Loop Cleanup
            delete CurPeak;
        }

        // Boundary Condtions
        EngX.insert(EngX.begin(), 0.0);
        EngDiff.insert(EngDiff.begin(),0.0);
        EngX.insert(EngX.end(), 5000.0);
        EngDiff.insert(EngDiff.end(),0.0);

        // Make TSpines
        TSpline *TempSpline =
            new TSpline3("Energy Offset", EngX.data(), EngDiff.data(), nPeaks);
        NonLinearityList->Add(TempSpline);
        pGriff->LoadEnergyResidual(i, TempSpline);

        // Make TGraph part of summary
        TGraph* TempGraph = new TGraph(nPeaks, EngX.data(), EngDiff.data());
        NonLinearityTGraph->Add(TempGraph);
        //ResidualCryst[ static_cast <int>( std::floor( i / 4.0 ) )]->Add(ResidualChan[i]);
    }

    printf("Overwriting energy matrix\n");
    // Load in Energy data
    if (gIsFragmentFile)
        pTree->Project("mat_en", "TFragment.GetEnergy():"
                                 "TFragment.GetChannelNumber()");
    else
        pTree->Project("mat_en",
                       "TGriffin.fGriffinLowGainHits.GetEnergy():"
                       "TGriffin.fGriffinLowGainHits.GetChannel().fNumber");

    // Construct Residual Summary
    //TCanvas* CResidualSum = new TCanvas();
    //for( int i = 0 ; i < 16 ; i++ )
    //    ResidualCryst[i]->Write();

    printf("Writing Energy Matrix\n");
    mat_en->Write();
    //NonLinearityList->Write("Nonlinearities", TObject::kSingleKey);
    printf("Writing Non-Linarities\n");
    NonLinearityList->Write();
    NonLinearityTGraph->Write();
    printf("Writing Graphs\n");

    // Project Matrix
    // Cleanup
    delete mat_en;
    pFile->Close();
}
