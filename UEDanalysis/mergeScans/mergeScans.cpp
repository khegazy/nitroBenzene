#include "../analysis.h"
#include "mergeTools/mergeClass.h"
#include <TLegend.h>

using namespace std;


int main(int argc, char* argv[]) {

  if (argc<2) {
    cerr<<"ERROR: Missing input arguments, must run code ./analysis.exe 'fileList.txt' !!!"<<endl;
    cerr<<"         Can also run ./analysis.exe 'fileList.txt' 'treeName'"<<endl;
    exit(0);
  }


  ///////////////////////////////////////////////////////////
  /////  Load environment and get the number of events  /////
  ///////////////////////////////////////////////////////////
  
  string fileList(argv[1]);
  string treeName("physics");
  if (argc==3) string treeName(argv[2]);
  analysisClass analysis(fileList, treeName);  // Use this when specifying treeName

  uint64_t Nentries;
  Nentries = analysis.setupEnvironment();  // Alter this function as needed for specific setup

  cout.setf(ios::scientific);
  
  //////////////////////////////////
  /////  Setting up variables  /////
  //////////////////////////////////

  auto iPos = fileList.find("run-");
  if (iPos == string::npos) {
    iPos = fileList.find("RUN-");
  }
  iPos += 4;
  std::string runName = fileList.substr(iPos, 13);
  cout<<"RunName: "<<runName<<endl;
  mergeClass merge(runName);

  std::string prefix = "";
  bool scanSearch = false;
  /////  Importing variables from command line  /////
  for (int iarg=2; iarg<argc; iarg+=2) {
    if (strcmp(argv[iarg],"-Odir") == 0) {
      string str(argv[iarg+1]); 
      merge.mergeScansOutputDir = str;
    }
    else if (strcmp(argv[iarg],"-ScanSearch") == 0) {
      string str(argv[iarg+1]);
      prefix = str + "-";
      merge.mergeScansOutputDir = merge.scanSearchOutputDir;
      scanSearch = true;
    }
    else if (strcmp(argv[iarg], "-OdirSuffix") == 0) {
      string str(argv[iarg+1]);
      merge.mergeScansOutputDir += (str + "/");

      if (!tools::fileExists(merge.mergeScansOutputDir)) {
        system(("mkdir " + merge.mergeScansOutputDir).c_str());
      }
    }
    else if (strcmp(argv[iarg], "-CompareRef") == 0) {
      std::string str(argv[iarg+1]);
      merge.compareReference(str);
    }
    else {
      cerr<<"ERROR!!! Option "<<argv[iarg]<<" does not exist!"<<endl;
      exit(0);
    }
  }

  /////  Plotting variables  /////
  std::vector<PLOToptions> opts(5);
  std::vector<std::string> vals(5);
  std::vector<PLOToptions> oppts(3);
  std::vector<std::string> vaals(3);
  oppts[0] = yLabel;   vaals[0] = "Time [ps]";
  oppts[1] = xLabel;   vaals[1] = "Scattering Q [arb units]";
  oppts[2] = draw;     vaals[2] = "COLZ";
  opts[0] = yLabel;   vals[0] = "Time [ps]";
  opts[1] = xLabel;   vals[1] = "Scattering Q [arb units]";
  opts[2] = draw;     vals[2] = "COLZ";
  opts[3] = minimum;  vals[3] = "-0.1";
  opts[4] = maximum;  vals[4] = "0.1";

  std::map<string, double > cbar;
  cbar["20161102_LongScan1_Leg0"] = 0.2;
  cbar["20161102_LongScan1_Leg2"] = 0.2;

  std::map< string, std::vector<double> > refAutCor, refLineOut;


  ///////////////////////////
  /////  Merging Scans  /////
  ///////////////////////////

  if (merge.verbose)  {
    std::cout << "Begin to merge runs" << endl;
  }

  std::vector<string> runInds;
  std::map< string, std::map< double, double* > >  diffP_arrays;
  std::map< string, int > runShifts;

  ///// Loop through events in the file and saving to maps  /////
  //int pInd;
  //int tmInd = 0;
  std::vector< std::vector<double> > rpVals(merge.NradAzmBins/5);
  for (uint64_t ievt=0; ievt<Nentries; ievt++) {
    analysis.loadEvent(ievt);


    //if (scan >= 75 && scan < 105) {
    //  continue;
    //}
    if (std::find(merge.badScans.begin(), merge.badScans.end(), scan) 
          != merge.badScans.end()) continue;
    auto badImgItr = merge.badImages.find(scan);
    if (badImgItr != merge.badImages.end()) {
      if (std::find(badImgItr->second.begin(), badImgItr->second.end(), stagePos) 
            != badImgItr->second.end()) {
        continue;
      }
    }

    // Remove Q regions of certain stage positions
    merge.removeBadRegions(azmAvg, stagePos);

    bool fillLabParams = false;

    // Adding Reference Images
    if (std::find(merge.refSubtractStagePos.begin(), 
            merge.refSubtractStagePos.end(), stagePos) 
          != merge.refSubtractStagePos.end()) {
      if (merge.verbose) std::cout << "INFO: Adding reference image.\n";
    
      merge.addReference(scan, stagePos, timeStamp, azmAvg, legCoeffs, imgNorm);
      fillLabParams = true;
    }

    // Adding Time Dependent Images
    if (!imgIsRef) {
      merge.addEntry(scan, stagePos, timeStamp, azmAvg, legCoeffs, imgNorm);
      /*
      if (stagePos == 1542750) {
        int k = 0;
        for (int i=4; i<merge.NradAzmBins; i+=5) {
          rpVals[k].insert(rpVals[k].end(), 
              (*radPixDist)[i].begin(), 
              (*radPixDist)[i].end());
          k+=1;
        }
        if (k != rpVals.size()) {
          cout<<"SIZES: "<<k<<" "<<rpVals.size()<<endl;
        }
      }
      */
      fillLabParams = true;
    }

    // Adding image parameters
    if (fillLabParams) {
      merge.addLabTimeParameter(timeStamp, "scan", scan);
      merge.addLabTimeParameter(timeStamp, "stagePos", stagePos);
      merge.addLabTimeParameter(timeStamp, "imgNorm", imgNorm);
      merge.addLabTimeParameter(timeStamp, "centerRstdRatio", centerRstdRatio);
      merge.addLabTimeParameter(timeStamp, "centerCstdRatio", centerCstdRatio);
      for (uint i=0; i<imgRadSTD->size(); i++) {
        merge.addLabTimeParameter(
            timeStamp, 
            "imgRadSTD" + to_string(i), 
            (*imgRadSTD)[i]);

      }
      // Adding lab/setup parameters
      for (auto const & pvInd : merge.pvMap) {
        if (pvInd.first.compare("pressure") == 0) {
          merge.addLabTimeParameter(timeStamp, "pressure", pressure);
        }
        else if (pvInd.first.compare("UVcounts") == 0) {
          merge.addLabTimeParameter(timeStamp, "UVcounts", UVcounts);
        }
        else if (pvInd.first.compare("bunkerTemp") == 0) {
          merge.addLabTimeParameter(timeStamp, "bunkerTemp", bunkerTemp);
        }
        else if (pvInd.first.compare("highBayTemp") == 0) {
          merge.addLabTimeParameter(timeStamp, "highBayTemp", bunkerTemp);
        }
        else {
          std::cerr << "ERROR: Do not know how to add PV " 
            << pvInd.first << "!!!\n";
          exit(1);
        }
      }
    }

    if (merge.saveMergeIntermediates) {
      save::saveDat<double>(
          (*imgOrig),
          merge.saveMergeInterFolder 
            + "/imgOrig-" + runName + "_scan-" + to_string(scan)
            + "_stagePos-" + to_string(stagePos) + ".dat");
      save::saveDat<double>(
          (*imgSubBkg),
          merge.saveMergeInterFolder 
            + "/imgSubBkg-" + runName + "_scan-" + to_string(scan)
            + "_stagePos-" + to_string(stagePos) + ".dat");
    }

  }

  ///  Cut on lab time parameters  ///
  
  if (merge.verbose) {
    std::cout << "INFO: Cutting on lab time parameters.\n";
  }

  merge.basicGreaterThanCut("centerRstdRatio", 3);
  merge.basicGreaterThanCut("centerCstdRatio", 3);
  for (uint i=0; i<imgRadSTD->size(); i++) {
    merge.stdParamCut("imgRadSTD" + to_string(i), 3);
  }

  ///  Subtract low order polynomials  ///
 
  if (merge.verbose) {
    std::cout << "INFO: Removing low order polynomials.\n";
  }

  merge.removeImgNormOutliers();
  merge.removeLowPolynomials();


  ///  Prune data for outliers and sparse time steps  ///

  if (merge.verbose) {
    std::cout << "INFO: Removing outliers.\n";
  }


  merge.scaleByFit();

  merge.removeOutliers();

  
  ///  Merging Scans  ///

  if (merge.verbose) {
    std::cout << "INFO: Merging Scans.\n";
  }

  merge.mergeScans();

  // Normalize line outs
  //merge.normalizeScansResults();

  // Get Mean, STD, and SEM
  merge.getRunMeanSTDSEM();


  /////  Saving  /////

  // Clean up NANVAL for saving and plotting
  for (int ir=0; ir<(int)merge.azmReference.size(); ir++) {
    if (merge.azmReference[ir] == NANVAL) {
      merge.azmReference[ir] = 0;
    }
  }
  for (int ilg=0; ilg<(int)merge.legReference.size(); ilg++) {
    for (int ir=0; ir<(int)merge.legReference[ilg].size(); ir++) {
      if (merge.legReference[ilg][ir] == NANVAL) {
        merge.legReference[ilg][ir] = 0;
      }
    }
  }

  if (merge.verbose) 
    std::cout << "INFO: saving merged references and before t0 subtraction.\n";
  // References
  save::saveDat<double>(merge.azmReference,
      merge.mergeScansOutputDir + 
      "/data-" + runName + "-" + prefix+ 
      "referenceAzm[" +
      to_string(merge.NradAzmBins) + "].dat");
  save::saveDat<double>(merge.legReference,
      merge.mergeScansOutputDir + 
      "/data-" + runName + "-" + prefix+ 
      "referenceLeg[" + 
      to_string(merge.Nlegendres) + 
      "," + to_string(merge.NradLegBins) + "].dat");

  /*
  for (int i=0; i<rpVals.size(); i++) {
    double mean = std::accumulate(rpVals[i].begin(), rpVals[i].end(), 0);
    mean /= rpVals[i].size();

    double var = 0;
    for (int k=0; k<rpVals[i].size(); k++) {
      var += std::pow(mean - rpVals[i][k], 2);
    }
    var /= rpVals[i].size();
    var = std::sqrt(var);
    var /= std::sqrt((float)rpVals[i].size());

    cout<<"SEM check radius "<<i
      <<" SEM / STD/sqrt(N) / ratio: "<<merge.runAzmSEM[tmInd][4+i*5]
      <<"  "<<var<<"  "<<merge.runAzmSEM[tmInd][4+i*5]/var<<"  "<<rpVals[i].size()<<endl;
  }
  */


  /////  Time Domain Changes  /////

  // Subtract T0
  if (merge.verbose) std::cout << "INFO: Subtracting T0.\n";
  merge.subtractT0();

  /////  Normalize and get statistics  /////

  // Gaussian smooth
  if (merge.verbose) std::cout << "INFO: Gaussian Smearing Q.\n";
  //merge.gaussianFilterQ();

  // Make pair correlations
  if (merge.verbose) std::cout << "INFO: Making pair correlations.\n";
  //merge.makePairCorrs();

  // Get Mean, STD, and SEM
  if (merge.verbose) std::cout << "INFO: Calculating mean, STD, SEM.\n";
  if (merge.useBootstrapSEM) {
    if (merge.computeBootstrapSEM) {
      merge.bootstrapSEM();

      // Testing the number of bootstrap loops are needed
      if (merge.testMergeNbootStrap) {
        merge.testSEMbootstrap();
      }
    }
  }
  else {
    merge.getRunMeanSTDSEM();
  }

  // Normalize to get sM(s)
  if (merge.verbose) std::cout << "INFO: sMs Normalization.\n";
  merge.sMsNormalize();

  std::vector<double> smooth, smoothSMS;
  for (int it=0; it<(int)merge.azimuthalAvg.size(); it++) {
    smooth    = imgProc::gaussianSmooth1d(
                  merge.azimuthalAvg[it], 
                  merge.mergeGSmoothSTD, 
                  7*merge.mergeGSmoothSTD);
    smoothSMS = imgProc::gaussianSmooth1d(
                  merge.azimuthalsMs[it],
                  merge.mergeGSmoothSTD, 
                  7*merge.mergeGSmoothSTD);
    for (int iq=0; iq<merge.NradAzmBins; iq++) {
      merge.azimuthalAvg[it][iq] = smooth[iq];
      merge.azimuthalsMs[it][iq] = smoothSMS[iq];
    }
  }


  // Clean up NANVAL for saving and plotting
  for (int itm=0; itm<(int)merge.azimuthalAvg.size(); itm++) {
    for (int ir=0; ir<(int)merge.azimuthalAvg[itm].size(); ir++) {
      if (merge.azimuthalAvg[itm][ir] == NANVAL) merge.azimuthalAvg[itm][ir] = 0;
    }
  }

  // Smooth in time
  merge.smearTimeGaussian();
  //merge.smearTimeFFT();


  /////  Saving and plotting  /////
  if (merge.verbose) 
    std::cout << "INFO: saving merged references after t0 subtraction.\n";

  if (!(merge.computeBootstrapSEM && merge.SEMisBootstrap) 
      && !(!merge.useBootstrapSEM && !merge.SEMisBootstrap)) {
    std::cerr << "WARNING: Cannot save SEM because method and "
      << "SEMisBootstrap do not align!!!\n";
  }

  if (merge.pltVerbose) {
    std::vector<double> test(merge.azimuthalsMs[0].size());
    plt.print1d(merge.azmReference, "./plots/reference");
    plt.printRC(merge.azimuthalAvg, "./plots/azmAvg");
    if (merge.didSMSnormalize) {
      plt.printRC(merge.azimuthalsMs, "./plots/azmAvgSMS");
    for (uint i=0; i<test.size(); i++) {
      test[i] = merge.azimuthalAvg[27][i]/merge.sMsAzmNorm[i];
      if (i<50) {
        test[i] = 0;
      }
    }
    plt.print1d(test, "test1d");
    }
  }

  save::saveDat<double>(merge.azimuthalAvg, 
      merge.mergeScansOutputDir + "data-"
      + runName + "-" + prefix + "azmAvgDiff["
      + to_string(merge.azimuthalAvg.size()) + ","
      + to_string(merge.azimuthalAvg[0].size()) + "].dat");
  if (merge.computeBootstrapSEM || !merge.useBootstrapSEM) {
    save::saveDat<double>(merge.runAzmSEM,
        merge.mergeScansOutputDir + "data-"
        + runName + "-" + prefix + "azmAvgSEM[" 
        + to_string(merge.runAzmSEM.size()) + ","
        + to_string(merge.runAzmSEM[0].size()) + "].dat");
  }

  save::saveDat<double>(merge.azmReference,
      merge.mergeScansOutputDir + "data-"
      + runName + "-" + prefix + "azmReference[" 
      + to_string(merge.azmReference.size()) + "].dat");
  for (auto & aItr : merge.azmIndReference) {
    save::saveDat<double>(aItr.second,
        merge.mergeScansOutputDir + "data-"
        + runName + "_" + prefix + "reference-"
        + to_string(aItr.first) + "_bins[" 
        + to_string(merge.NradAzmBins) + "].dat");
  }

  if (merge.smearedTime) {
    save::saveDat<double>(merge.smearedAzmAvg, 
        merge.mergeScansOutputDir + "data-"
        + runName + "-" + prefix + "tSmeared-"
        + to_string(merge.timeSmearSTD) + "-azmAvgDiff["
        + to_string(merge.smearedAzmAvg.size()) + ","
        + to_string(merge.smearedAzmAvg[0].size()) + "].dat");
    
    if (merge.didSMSnormalize) {
      save::saveDat<double>(merge.smearedAzmsMs, 
        merge.mergeScansOutputDir + "data-"
        + runName + "-" + prefix + "tSmeared-"
        + to_string(merge.timeSmearSTD) + "-sMsAzmAvgDiff["
        + to_string(merge.smearedAzmAvg.size()) + ","
        + to_string(merge.smearedAzmAvg[0].size()) + "].dat");
    }
  }

  if (merge.didSMSnormalize) {
    save::saveDat<double>(merge.azimuthalsMs, 
        merge.mergeScansOutputDir + "data-"
        + runName + "-" + prefix + "sMsAzmAvgDiff["
        + to_string(merge.azimuthalAvg.size()) + ","
        + to_string(merge.azimuthalAvg[0].size()) + "].dat");
    if (merge.computeBootstrapSEM || !merge.useBootstrapSEM) {
      save::saveDat<double>(merge.runsMsMeans,
          merge.mergeScansOutputDir + "data-"
          + runName + "-" + prefix + "sMsMean[" 
          + to_string(merge.runAzmMeans.size()) + ","
          + to_string(merge.runAzmMeans[0].size()) + "].dat");

      save::saveDat<double>(merge.runsMsSEM,
          merge.mergeScansOutputDir + "data-"
          + runName + "-" + prefix + "sMsSEM[" 
          + to_string(merge.runAzmSEM.size()) + ","
          + to_string(merge.runAzmSEM[0].size()) + "].dat");

      save::saveDat<double>(merge.runsMsRefMean,
          merge.mergeScansOutputDir + "data-"
          + runName + "-" + prefix
          + "referenceAzmsMsMean[" 
          + to_string(merge.NradAzmBins) + "].dat");

      save::saveDat<double>(merge.runsMsRefSEM,
          merge.mergeScansOutputDir + "data-"
          + runName + "-" + prefix
          + "referenceAzmsMsSEM[" 
          + to_string(merge.NradAzmBins) + "].dat");
    }
  }
 
  if (merge.didPairCorrSTD) {
    if (merge.computeBootstrapSEM || !merge.useBootstrapSEM) {
      save::saveDat<double>(merge.runPCorrSEM,
          merge.mergeScansOutputDir + "data-"
          + runName + "-" + prefix + "pairCorrSEM["
          + to_string(merge.runPCorrSEM.size()) + ","
          + to_string(merge.runPCorrSEM[0].size()) + "].dat");
    }
  }

  if (merge.saveMergeIntermediates) {
    for (auto & sRitr : merge.scanReferences) {
      for (auto & pItr : sRitr.second) {
        if (pItr.second.scale) {
          save::saveDat<double>(
              pItr.second.azmRef, 
              merge.saveMergeInterFolder 
                + "/azmAvgRef-" + runName + "_scan-" + to_string(sRitr.first)
                + "_stagePos-" + to_string(pItr.first) + ".dat");
        }
      }
    }

    for (auto & sAzml : merge.scanAzmAvg) {
      for (auto & pItr : merge.stagePosInds) {
        if (merge.scanScale[sAzml.first][pItr.second] != 0) {
          save::saveDat<double>(
              sAzml.second[pItr.second],
              merge.saveMergeInterFolder 
                + "/azmAvg-" + runName + "_scan-" + to_string(sAzml.first)
                + "_stagePos-" + to_string(pItr.first) + ".dat");
        }
      }
    }
  }


  if (scanSearch) {
    plt.printRC(merge.azimuthalAvg, 
        merge.mergeScansOutputDir + "/plots/data-"
        + runName + "-" + prefix + "azmAvgDiff", opts, vals);
  }


  /////////////////////////
  /////  Cleaning up  /////
  /////////////////////////

  if (merge.verbose) {
    std::cout << "Cleaning up" << endl;
  }

  return 1;
}
