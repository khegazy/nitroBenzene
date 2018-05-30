#include "simulations.h"


int main(int argc, char* argv[]) {

  radicalEnums molecule = nitrobenzene;

  // Diffraction Pattern
  uint Nbins = 30;
  //double maxQ = 11.3*((double)Nbins)/1000.0;
  double maxQ = 11.3;
  double elEnergy = 3.7e6;
  double Iebeam = 5;
  double screenDist = 4;


  PLOTclass plt;

  double seed = (double)clock();
  int Nmols = 1;
  int index = 0;
  std::string fileName = "NBZrefDiff";
  std::string outputDir = "output/references";
  //string outputDir = "/reg/neh/home/khegazy/simulations/n2o/diffractionPatterns/output";
  if (argc > 1) {
    Nmols = atoi(argv[1]);
  }
  if (argc > 2) {
    for (int iarg=2; iarg<argc; iarg+=2) {
      if (strcmp(argv[iarg],"-Ofile")==0) {string str(argv[iarg+1]); fileName=str;}
      else if (strcmp(argv[iarg],"-Odir")==0) {string str(argv[iarg+1]); outputDir=str;}
      else if (strcmp(argv[iarg],"-Index")==0) {index = atoi(argv[iarg+1]);}
      else {
        cerr<<"ERROR!!! Option "<<argv[iarg]<<" does not exist!"<<endl;
        exit(0);
      }
    }
  }

  ////////////////////////////////////////
  /////  Making Diffraction Pattern  /////
  ////////////////////////////////////////

  std::vector<MOLENSEMBLEMCclass*> molMCs;

  switch(molecule) {
    case nitrobenzene: {
      /////  nitrobenzene  /////
      NBZMCclass* NBZmc = new NBZMCclass(seed);
      NBZmc->Nmols = Nmols;
      NBZmc->NmolAtoms = 14;
      NBZmc->atomTypes.push_back(N);
      NBZmc->atomTypes.push_back(C);
      NBZmc->atomTypes.push_back(O);
      NBZmc->atomTypes.push_back(H);
      molMCs.push_back(NBZmc);
      break;
    }

    case phenoxyRadical: {  
      /////  phenoxy radical  /////
      PNOXYRadMCclass* PNOXYmc = new PNOXYRadMCclass(seed);
      PNOXYmc->Nmols = Nmols;
      PNOXYmc->NmolAtoms = 12;
      PNOXYmc->atomTypes.push_back(C);
      PNOXYmc->atomTypes.push_back(O);
      PNOXYmc->atomTypes.push_back(H);
      molMCs.push_back(PNOXYmc);

      NOMCclass* NOmc = new NOMCclass(seed);
      NOmc->Nmols = Nmols;
      NOmc->NmolAtoms = 2;
      NOmc->atomTypes.push_back(O);
      NOmc->atomTypes.push_back(N);
      molMCs.push_back(NOmc);
      break;
    }

    case phenylRadical: {     
      /////  phenyl radical  /////
      PNLRadMCclass* PNLmc = new PNLRadMCclass(seed);
      PNLmc->Nmols = Nmols;
      PNLmc->NmolAtoms = 11;
      PNLmc->atomTypes.push_back(C);
      PNLmc->atomTypes.push_back(H);
      molMCs.push_back(PNLmc);

      NO2MCclass* NO2mc = new NO2MCclass(seed);
      NO2mc->Nmols = Nmols;
      NO2mc->NmolAtoms = 3;
      NO2mc->atomTypes.push_back(O);
      NO2mc->atomTypes.push_back(N);
      molMCs.push_back(NO2mc);
      break;
    }

    default:                
      cerr << "ERROR: do not recognize molecule enum!!!\n";
      exit(0);
  }


  std::vector< std::vector<double> > curLineOuts;
  std::map< std::string, std::vector<double> > lineOuts;
  std::map< std::string, std::vector< std::vector<double> > > diffPatterns;
  for (auto mc : molMCs) {
    mc->useOrientationMC = false;
    mc->makeMolEnsemble();
    cout<<"Made molecular ensemble"<<endl;

    DIFFRACTIONclass diffP(mc, maxQ, Iebeam, screenDist, elEnergy, Nbins,
        "/reg/neh/home/khegazy/simulations/scatteringAmplitudes/3.7MeV/");
    cout<<"Declared diffraction class"<<endl;
    
    curLineOuts.clear();
    curLineOuts = diffP.diffPatternCalc_uniform();
    cout<<"Made diffraction patterns."<<endl;

    // fill diffraction patterns
    if (diffPatterns.size() == 0) {
      diffPatterns["diffractionPattern"]    = diffP.diffPattern;
      diffPatterns["molDiffractionPattern"] = diffP.diffMolPattern;
      diffPatterns["atmDiffractionPattern"] = diffP.diffAtmPattern;
      diffPatterns["sPattern"]              = diffP.sPattern;
    }
    else {
      for (uint ir=0; ir<diffP.diffPattern.size(); ir++) {
        for (uint ic=0; ic<diffP.diffPattern[ir].size(); ic++) {
          diffPatterns["diffractionPattern"][ir][ic]    += diffP.diffPattern[ir][ic];
          diffPatterns["molDiffractionPattern"][ir][ic] += diffP.diffMolPattern[ir][ic];
          diffPatterns["atmDiffractionPattern"][ir][ic] += diffP.diffAtmPattern[ir][ic];
        }
      }
    }

    // fill lineouts
    if (lineOuts.size() == 0) {
      lineOuts["diffractionPattern"]    = curLineOuts[0];
      lineOuts["molDiffractionPattern"] = curLineOuts[1];
      lineOuts["atmDiffractionPattern"] = curLineOuts[2];
      lineOuts["sPattern"]              = curLineOuts[3];
    }
    else {
      for (uint i=0; i<lineOuts[0].size(); i++) {
        lineOuts["diffractionPattern"][i]    += curLineOuts[0][i];
        lineOuts["molDiffractionPattern"][i] += curLineOuts[1][i];
        lineOuts["atmDiffractionPattern"][i] += curLineOuts[2][i];
      }
    }
  }


  /////////////////////////////////////////
  /////  Plotting and Saving Results  /////
  /////////////////////////////////////////

  std::string prefix = outputDir + "/" + fileName + "_";
  std::string suffix = "Bins-" + to_string(Nbins) + "_Qmax-" + to_string(maxQ)
                        + "_Ieb-" + to_string(Iebeam) + "_scrnD-"
                        + to_string(screenDist) + "_elE-" + to_string(elEnergy);

  std::string lineOutSpan = "0," + to_string(maxQ);
  std::vector<PLOToptions> opts(3);
  std::vector<std::string> vals(3);
  opts[0] = xSpan;      vals[0] = to_string(-maxQ) + "," + to_string(maxQ);
  opts[1] = ySpan;      vals[1] = to_string(-maxQ) + "," + to_string(maxQ);
  opts[2] = fileType;   vals[2] = "png";


  delete (TH2F*)plt.printRC(diffPatterns["diffractionPattern"], 
      prefix + "diffractionPattern_" + suffix, opts, vals);
  delete (TH2F*)plt.printRC(diffPatterns["molDiffractionPattern"], 
      prefix + "molDiffractionPattern_" + suffix, opts, vals);
  delete (TH2F*)plt.printRC(diffPatterns["atmDiffractionPattern"], 
      prefix + "atmDiffractionPattern_" + suffix, opts, vals);
  delete (TH2F*)plt.printRC(diffPatterns["sPattern"], 
      prefix + "sPattern_" + suffix, opts, vals);
  delete (TH1F*)plt.print1d(lineOuts["diffractionPattern"], 
      prefix + "diffractionPatternLineOut_" + suffix, xSpan, lineOutSpan);
  delete (TH1F*)plt.print1d(lineOuts["molDiffractionPattern"], 
      prefix + "molDiffractionPatternLineOut_" + suffix, xSpan, lineOutSpan);
  delete (TH1F*)plt.print1d(lineOuts["atmDiffractionPattern"], 
      prefix + "atmDiffractionPatternLineOut_" + suffix, xSpan, lineOutSpan);
  delete (TH1F*)plt.print1d(lineOuts["sPattern"], 
      prefix + "sPatternLineOut_" + suffix, xSpan, lineOutSpan);


  FILE* otpDp = fopen((prefix + "diffractionPattern_" + suffix + ".dat").c_str(), "wb");
  FILE* otpAp = fopen((prefix + "atmDiffractionPattern_" + suffix + ".dat").c_str(), "wb");
  FILE* otpMp = fopen((prefix + "molDiffractionPattern_" + suffix + ".dat").c_str(), "wb");
  FILE* otpSp = fopen((prefix + "sPattern_" + suffix + ".dat").c_str(), "wb");
  FILE* otpDpLo = fopen((prefix + "diffractionPatternLineOut_" + suffix + ".dat").c_str(), "wb");
  FILE* otpMpLo = fopen((prefix + "molDiffractionPatternLineOut_" + suffix + ".dat").c_str(), "wb");
  FILE* otpApLo = fopen((prefix + "atmDiffractionPatternLineOut_" + suffix + ".dat").c_str(), "wb");
  FILE* otpSpLo = fopen((prefix + "sPatternLineOut_" + suffix + ".dat").c_str(), "wb");

  for (uint ir=0; ir<diffPatterns["diffractionPattern"].size(); ir++) {
    fwrite(&diffPatterns["diffractionPattern"][ir][0], sizeof(double), 
        diffPatterns["diffractionPattern"][ir].size(), otpDp);
    fwrite(&diffPatterns["molDiffractionPattern"][ir][0], sizeof(double), 
        diffPatterns["molDiffractionPattern"][ir].size(), otpMp);
    fwrite(&diffPatterns["atmDiffractionPattern"][ir][0], sizeof(double), 
        diffPatterns["atmDiffractionPattern"][ir].size(), otpAp);
    fwrite(&diffPatterns["sPattern"][ir][0], sizeof(double), 
        diffPatterns["sPattern"][ir].size(), otpSp);
  }

  fwrite(&lineOuts["diffractionPattern"][0], sizeof(double), 
      lineOuts["diffractionPattern"].size(), otpDpLo);
  fwrite(&lineOuts["molDiffractionPattern"][0], sizeof(double), 
      lineOuts["molDiffractionPattern"].size(), otpMpLo);
  fwrite(&lineOuts["atmDiffractionPattern"][0], sizeof(double), 
      lineOuts["atmDiffractionPattern"].size(), otpApLo);
  fwrite(&lineOuts["sPattern"][0], sizeof(double), 
      lineOuts["sPattern"].size(), otpSpLo);


  //////////////////////
  /////  Clean up  /////
  //////////////////////
  
  for (auto mc : molMCs) {
    delete mc;
  }

  fclose(otpDp);
  fclose(otpAp);
  fclose(otpMp);
  fclose(otpSp);
  fclose(otpDpLo);
  fclose(otpApLo);
  fclose(otpMpLo);
  fclose(otpSpLo);


  return 1;
}
