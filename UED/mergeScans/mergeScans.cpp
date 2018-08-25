#include "analysis.h"
//#include "../simulation/diffractionPattern/simulations.h"
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
  auto fPos = fileList.find(".txt");
  std::string runName = fileList.substr(iPos, 13); //fPos - iPos);
  cout<<"RunName: "<<runName<<endl;
  alignClass align(runName);
  //parameterClass params(runName);

  std::string prefix = "";
  bool scanSearch = false;
  /////  Importing variables from command line  /////
  for (int iarg=2; iarg<argc; iarg+=2) {
    if (strcmp(argv[iarg],"-Odir") == 0) {
      string str(argv[iarg+1]); 
      align.alignScansOutputDir = str;
    }
    else if (strcmp(argv[iarg],"-ScanSearch") == 0) {
      string str(argv[iarg+1]);
      prefix = str + "-";
      align.alignScansOutputDir = align.scanSearchOutputDir;
      scanSearch = true;
    }
    else if (strcmp(argv[iarg], "-OdirSuffix") == 0) {
      string str(argv[iarg+1]);
      align.alignScansOutputDir += (str + "/");

      if (!tools::fileExists(align.alignScansOutputDir)) {
        system(("mkdir " + align.alignScansOutputDir).c_str());
      }
    }
    else {
      cerr<<"ERROR!!! Option "<<argv[iarg]<<" does not exist!"<<endl;
      exit(0);
    }
  }

  cout<<"here1"<<endl;
  std::vector<double> reference;
  std::vector<double> referenceCount;

  int FFTsize = align.NradLegBins*2 + align.NautCpadding + 1;
  //int FFTsize = (NdiffInds + NautCpadding)*2 + 1;
  int FTsize = (int)(FFTsize/2) + 1;
  int indHR =  align.holeRat*align.NradLegBins;
  int outSize = FTsize*align.rMaxAzmRat;


  double* qSpace = (double*) fftw_malloc(sizeof(double)*FFTsize);
  fftw_complex* rSpace =
        (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(int)((FFTsize/2) + 1));
  fftw_plan fftB;
  fftB = fftw_plan_dft_r2c_1d(FFTsize, qSpace, rSpace, FFTW_MEASURE);

  cout<<"here2"<<endl;
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
  opts[3] = minimum;  vals[3] = "-0.2";
  opts[4] = maximum;  vals[4] = "0.2";

  std::map<string, double > cbar;
  cbar["20161102_LongScan1_Leg0"] = 0.2;
  cbar["20161102_LongScan1_Leg2"] = 0.2;

  std::map< string, std::vector<double> > refAutCor, refLineOut;

  int curScan;
  
  ////////////////////////////////////////
  /////  Compare simulation results  /////
  ////////////////////////////////////////
  
  if (align.compareSims) {
    align.compareSimulations(radicalNames);
  }

  ///////////////////////////
  /////  Aligning Runs  /////
  ///////////////////////////

  if (align.verbose)  {
    std::cout << "Begin to align runs" << endl;
  }

  std::vector<string> runInds;
  std::map< string, std::map< double, double* > >  diffP_arrays;
  std::map< string, int > runShifts;

  ///// Loop through events in the file and saving to maps  /////
  curScan = -1;
  int pInd;
  for (uint64_t ievt=0; ievt<Nentries; ievt++) {
    analysis.loadEvent(ievt);

    if (std::find(align.badScans.begin(), align.badScans.end(), scan) 
          != align.badScans.end()) continue;

    align.addLabTimeParameter(timeStamp, scan, stagePos, imgNorm);

    // Ignore reference images taken before scan
    if (imgIsRef) {
      if (align.verbose) std::cout << "INFO: Adding reference image.\n";

      align.addReference(scan, stagePos, azmAvg, legCoeffs, imgNorm);
      continue;
    }

    ///  Insert entries to respective maps  ///
    align.addEntry(scan, stagePos, azmAvg, legCoeffs, imgNorm);

  }




  ///////////////////////////////////////////////////////////
  /////  Prune data for outliers and sparse time steps  /////
  ///////////////////////////////////////////////////////////


  //cout<<"removing outliers"<<endl;
  //align.removeOutliers();

  cout<<"merging scans"<<endl;
  align.mergeScans();

  /////  Saving  /////
  // References
  save::saveDat<double>(align.azmReference,
      "./results/referenceAzm-" + runName +
      "[" + to_string(align.NradAzmBins) + "].dat");
  save::saveDat<double>(align.legReference,
      "./results/referenceLeg-" + runName + 
      "[" + to_string(align.Nlegendres) + 
      "," + to_string(align.NradLegBins) + "].dat");

  // Raw diffraction images
  save::saveDat<double>(align.azimuthalAvg, 
      align.alignScansOutputDir + "data-"
      + runName + "-" + prefix + "sMsAzmAvgDiffRaw["
      + to_string(align.azimuthalAvg.size()) + ","
      + to_string(align.azimuthalAvg[0].size()) + "].dat");
 
  /*
  if (scanSearch || align.pltVerbose) {
    plt.printRC(align.azimuthalAvg, 
        "data-"
        + runName + "-" + prefix + "sMsAzmAvgDiffRaw", maximum, "1e-18");
  }
  */
  

  cout<<"subtracting t0 and norm"<<endl;
  align.subtractT0andNormalize();

  cout<<"smearing time"<<endl;
  //align.smearTime();


  // Clean up NANVAL for saving and plotting
  for (int itm=0; itm<align.azimuthalAvg.size(); itm++) {
    for (int ir=0; ir<align.azimuthalAvg[itm].size(); ir++) {
      if (align.azimuthalAvg[itm][ir] == NANVAL) align.azimuthalAvg[itm][ir] = 0;
    }
  }


  save::saveDat<double>(align.azimuthalAvg, 
      align.alignScansOutputDir + "data-"
      + runName + "-" + prefix + "sMsAzmAvgDiff["
      + to_string(align.azimuthalAvg.size()) + ","
      + to_string(align.azimuthalAvg[0].size()) + "].dat");
  plt.printRC(align.azimuthalAvg, 
      "./plots/data-"
      + runName + "-" + prefix + "sMsAzmAvgDiff", opts, vals);

  if (scanSearch) {
    cout<<"plotting to "<<align.alignScansOutputDir + "/plots/data-"+ runName + "-" + prefix + "sMsAzmAvgDiff";
    plt.printRC(align.azimuthalAvg, 
        align.alignScansOutputDir + "/plots/data-"
        + runName + "-" + prefix + "sMsAzmAvgDiff", opts, vals);
  }


  /*
  for (int ilg=0; ilg<align.Nlegendres; ilg++) {
    ///  Saving data  ///
    cout<<"111"<<endl;
    save::saveDat<double>(align.legendres[ilg], 
        align.alignScansOutputDir + "data-"
        + runName + "-" + prefix + "sMsL"
        + to_string(ilg) + "Diff["
        + to_string(align.legendres[ilg].size()) + ","
        + to_string(align.legendres[ilg][0].size()) + "].dat");
    cout<<"222"<<endl;
    save::saveDat<double>(align.legendres[ilg][align.legendres[ilg].size()-1], 
        align.alignScansOutputDir + "data-"
        + runName + "-" + prefix + "sMsFinalL"
        + to_string(ilg) + "Diff["
        + to_string(align.legendres[ilg][0].size()) + "].dat");


    cout<<"333"<<endl;
    save::saveDat<double>(align.smearedImg[ilg], 
        align.alignScansOutputDir + "data-"
        + runName + "-" + prefix + "sMsL"
        + to_string(ilg) + "DiffSmear["
        + to_string(align.smearSTD) + "["
        + to_string(align.smearedImg[ilg].size()) + ","
        + to_string(align.smearedImg[ilg][0].size()) + "].dat");

    cout<<"444"<<endl;
    save::saveDat<double>(align.smearedImg[ilg][align.stagePosInds.size()-1],
        align.alignScansOutputDir + "data-"
        + runName + "-" + prefix + "sMsFinalL"
        + to_string(ilg) + "DiffSmear"
        + to_string(align.smearSTD) + "["
        + to_string(align.smearedImg[ilg][0].size()) + "].dat");
  }
  */



  cout<<"start autocorrelation"<<endl;

  ///////////////////////////////////////
  /////  Pair correlation function  /////
  ///////////////////////////////////////
  if (align.verbose) {
    std::cout << "Begin calculating pair correlation functions\n";
  }

  int padding = 500;
  //std::vector<double> fftInp(2*params.NradAzmBins + 2*padding + 1);



  /*
  std::vector< std::vector<double> > pairCorr(smearedImg[0].size()), pairCorr1d;
  std::vector<double> powerSpct(outSize);
  for (int it=0; it<NtimeSteps; it++) {
    pairCorr[it].resize(outSize);

    std::vector<double> inpDiff(smearedImg[0][0].size()*2+1, 0);
    int centI = (int)(inpDiff.size()/2);
    int indHR =  holeRat*smearedImg[0][it].size();
    for (int ir=0; ir<(int)smearedImg[0][it].size(); ir++) {
      if (ir < indHR) {
        inpDiff[centI+1+ir] = smearedImg[0][it][indHR]
              *pow(sin((PI/2)*((ir+1)/((double)(indHR+1)))), 2);
        //inpDiff[centI+1+ir] = smearedImg[0][it][ir]
        //      *pow(sin((PI/2)*((ir+1)/((double)(indHR+1)))), 2);
        inpDiff[centI-1-ir] = inpDiff[centI+1+ir];
      }
      else {
        inpDiff[centI+1+ir] = smearedImg[0][it][ir];
        inpDiff[centI-1-ir] = smearedImg[0][it][ir];
      }
    }

    pairCorr1d = tools::fft1dRtoC(inpDiff, rMaxRat, NautCpadding, 
          padDecayRat, fftB, qSpace, rSpace, false);

    // Retrieve result and save results
    for (int ir=0; ir<outSize; ir++) {
      pairCorr[it][ir] = pairCorr1d[0][ir]; 
    }
  }

  // Saving pair correlation
  save::saveDat<double>(pairCorr, "./plots/data/data_pairCorrDiffSmear"
      + to_string(stdev) + "["
      + to_string(pairCorr.size()) + ","
      + to_string(pairCorr[0].size()) + "].dat");
  save::saveDat<double>(pairCorr[pairCorr.size()-1], 
      "./plots/data/data_pairCorrFinalDiffSmear" 
      + to_string(stdev) + "["
      + to_string(pairCorr[0].size()) + "].dat");
  */


  /////////////////////////
  /////  Cleaning up  /////
  /////////////////////////

  if (align.verbose) {
    std::cout << "Cleaning up" << endl;
  }

  fftw_destroy_plan(fftB);
  fftw_free(rSpace);
  fftw_free(qSpace);
  return 1;
}