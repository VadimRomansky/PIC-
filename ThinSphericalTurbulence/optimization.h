#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

//new scheme
double evaluateOptimizationFunction5(double* vector, double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length);
void findMinParameters5(double* vector, const double* grad, double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, double& currentF);
void optimizeParameters5(double* vector,  double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, FILE* logFile);



double evaluateOptimizationFunction5(double Bfactor, double n, double fractionSize, double rmax, double v, double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length);
void findMinParameters5(const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v, double gradB, double gradN, double gradS, double gradR, double gradV, double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, double& currentF);
void optimizeParameters5(const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v,  double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, FILE* logFile);
//for simple flat disk
double evaluateOptimizationFunction5simple(double Bfactor, double n, double fractionSize, double rmax, double v, double** Ee, double** Fe, int Np, int Nd, double sintheta, int thetaIndex);
void findMinParameters5simple(const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v, double gradB, double gradN, double gradS, double gradR, double gradV, double** Ee, double** dFe, int Np, int Nd, double sintheta, int thetaIndex, double& currentF);
void optimizeParameters5simple(const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v, double** Ee, double** dFe, int Np, int Nd, double sintheta, int thetaIndex, FILE* logFile);

//for fix sigma
void findMinParameters5sigma(const double& sigma, const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v, double gradB, double gradS, double gradR, double gradV, double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, double& currentF);
void optimizeParameters5sigma(const double& sigma, const double& B0, const double& N0, const double& R0, const double& V0, double& Bfactor, double& N, double& fractionSize, double& rmax, double& v,  double** nu, double** observedInu, double** Ee, double** dFe, int Np, int Nnu, int Nd, int Nmonth, double*** Bn, double*** sintheta, int*** thetaIndex, double*** concentrations, double***** Inu, double***** Anu, double*** area, double*** length, FILE* logFile);
#endif