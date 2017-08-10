#ifndef SPMERGE_H
#define SPMERGE_H

#include<vector>
#include<gsl/gsl_rng.h>
#include<gsl/gsl_randist.h>
#include"Pythia8/Pythia.h"

using namespace Pythia8;

class Lorentz{
  private :

	double **Lambda;
	double **Mrot;

  public :

	Lorentz();
	~Lorentz();

	void Boost1_RestToLab(int dim, double *U, double *Vin, double *Vout);
	void Boost1_LabToRest(int dim, double *U, double *Vin, double *Vout);
	void Boost2_RestToLab(int dim, double *U, double **Tin, double **Tout);
	void Boost2_LabToRest(int dim, double *U, double **Tin, double **Tout);
	void TransRotation1(int dim, double phi, double *Vin, double *Vout);
	void TransRotation2(int dim, double phi, double **Tin, double **Tout);
};

class SPmerge{
  private :

	int PDGidA, PDGidB;
	int baryonA, baryonB;
	int *idqsetA;
	int *idqsetB;
	double massA, massB;
	double sqrtsAB, pabscomAB;
	double *plabA;
	double *plabB;
	double *pcomA;
	double *pcomB;
	double *ucomAB;
	double **evecBasisAB;

	int NpartFinal;
	int NpartString1;
	int NpartString2;

	double pLightConeMin;
	double xfracMin;

	double betapowS;

	double alphapowV;
	double betapowV;

	double sigmaQperp;

	double kappaString;

	int CINI, CFIN;
	int BINI, BFIN;
	double EINI, EFIN;
	double pxINI, pxFIN;
	double pyINI, pyFIN;
	double pzINI, pzFIN;

	const gsl_rng_type *Type;
	gsl_rng *rng;
	Lorentz *lorentz;
	Pythia *pythia;

	double XSecTot, XSecEl, XSecInel, XSecAX, XSecXB, XSecXX, XSecAXB, XSecND;
	double *XSecSummed;
	SigmaTotal sigmaTot;

  public :

	SPmerge();
	~SPmerge();

	// final state array
	vector<int> *final_PDGid;
	// final_PDGid[0] : PDGid
	// final_PDGid[1] : if it is leading hadron (1 = Yes, 0 = No)
	vector<double> *final_pvec;
	// final_pvec[0] : energy
	// final_pvec[1] : px
	// final_pvec[2] : py
	// final_pvec[3] : pz
	// final_pvec[4] : mass
	vector<double> *final_tform;
	// final_tform[0] : formation time (fm)
	// final_tform[1] : suppression factor for the cross section (xtotfac)

	void set_gsl_rng_type(const gsl_rng_type *TypeIn){
		Type = TypeIn;
	}
	void set_gsl_rng(gsl_rng *rngIn){
		rng = rngIn;
	}
	void set_pythia(Pythia *pythiaIn);

	void set_pLightConeMin(double pLightConeMinIn){pLightConeMin = pLightConeMinIn;}

	void set_betapowS(double betapowSIn){betapowS = betapowSIn;}

	void set_alphapowV(double alphapowVIn){alphapowV = alphapowVIn;}
	void set_betapowV(double betapowVIn){betapowV = betapowVIn;}

	void set_sigmaQperp(double sigmaQperpIn){sigmaQperp = sigmaQperpIn;}

	void set_kappaString(double kappaStringIn){kappaString = kappaStringIn;}

	bool init_lab(int idAIn, int idBIn, double massAIn, double massBIn, Vec4 pvecAIn, Vec4 pvecBIn);
	bool init_com(int idAIn, int idBIn, double massAIn, double massBIn, double sqrtsABIn);

	bool next_Inel();
	bool next_SDiff_AX(); // single-diffractive : A + B -> A + X
	bool next_SDiff_XB(); // single-diffractive : A + B -> X + B
	bool next_DDiff_XX(); // double-diffractive : A + B -> X + X
	bool next_NDiff(); // non-diffractive
	//bool next_BBarAnn(); // baryon-antibaryon annihilation

	double get_XSecTot(){return XSecTot;}
	double get_XSecEl(){return XSecEl;}
	double get_XSecInel(){return XSecInel;}
	double get_XSecAX(){return XSecAX;}
	double get_XSecXB(){return XSecXB;}
	double get_XSecXX(){return XSecXX;}
	double get_XSecAXB(){return XSecAXB;}
	double get_XSecND(){return XSecND;}

	void reset_finalArray();
	int append_finalArray(double *uString, double *evecLong);
	bool check_conservation();

	void PDGid2idqset(int pdgid, int *idqset);
	void makeStringEnds(int *idqset, int *idq1, int *idq2);
	int fragmentString(int idq1, int idq2, double mString, double *evecLong, bool ranrot);

	double sample_XSDIS(double xmin, double beta);
	double sample_XVDIS(double xmin, double alpha, double beta);
	double sample_Qperp(double sigQ);
};

#endif

