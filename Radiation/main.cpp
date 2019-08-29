#include "stdio.h"
#include <stdlib.h>
#include <time.h>
#include "math.h"
#include <string>

const double kBoltzman = 1.3806488E-16;
const double speed_of_light = 2.99792458E10;
const double speed_of_light2 = speed_of_light * speed_of_light;
const double speed_of_light4 = speed_of_light2 * speed_of_light2;
const double electron_charge = 4.803529695E-10;
const double pi = 4 * atan2(1.0, 1.0);
const double massElectronReal = 0.910938291E-27;
const double massProtonReal = 1.67262177E-24;
const double massRatio = 25;
//const double massElectron = massProtonReal/massRatio;
const double massElectron = massElectronReal;

const int Napprox = 40;

const double McDonaldValue[Napprox] = {
	3.08E8, 2.11E7, 6.65E6, 4.55E5, 1.43E4, 9802, 3087, 670, 211, 107, 66.3, 33.6, 20.7, 14.1, 11.0, 10.3, 6.26, 4.2, 3.01, 2.25, 1.73, 1.37, 1.10,
	0.737, 0.514, 0.368, 0.269, 0.2, 0.0994, 0.0518, 0.0278, 0.0152, 0.00846, 0.00475, 0.00154, 0.000511, 0.000172, 0.0000589, 0.0000203, 0.00000246
};
const double UvarovValue[Napprox] = {
	0.0461, 0.0791, 0.0995, 0.169, 0.213, 0.358, 0.445, 0.583, 0.702, 0.772, 0.818, 0.874,0.904, 0.917, 0.918, 0.918, 0.901, 0.872, 0.832, 0.788,
	0.742, 0.694, 0.655, 0.566, 0.486, 0.414, 0.354, 0.301, 0.200, 0.130, 0.0845, 0.0541, 0.0339, 0.0214, 0.0085, 0.0033, 0.0013, 0.00050, 0.00019,
	0.0000282
};
const double UvarovX[Napprox] = {
	0.00001, 0.00005, 0.0001 ,0.0005, 0.001, 0.005, 0.01, 0.025, 0.050, 0.075, 0.10, 0.15,
	0.20, 0.25, 0.29, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 1.0, 1.2, 1.4, 1.6,
	1.8, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 12.0
};

const double augx[5] = {0.335, 0.625, 1.46, 4.92, 8.57};
const double augy[5] = {3.29, 7.77, 8.53, 2.42, 1.06};

const double augmaxx = 0.886;
const double augmaxy = 11.2;

const double junx[4] = {0.628, 1.45, 4.89, 8.53};
const double juny[4] = {2.98, 12.3, 5.79, 3.15};

const double junmaxx = 1.65;
const double junmaxy = 13.2;

const double mayx[3]= {1.46, 4.94, 8.62};
const double mayy[3] = {4.91, 12.0, 6.67};

const double maymaxx = 2.96;
const double maymaxy = 15.2;

const double aprx[4] = {1.44, 4.91, 8.58, 22.8};
const double apry[4] ={0.993, 13.9, 17.1, 5.11};

const double aprmaxx = 6.50;
const double aprmaxy = 19.3;

double power(const double& v, const double& p) {
	return exp(p * log(v));
}

double criticalNu(const double& E, const double& sinhi, const double& H) {
	return 3 * electron_charge * H * sinhi * E * E / (4 * pi * massElectron * massElectron * massElectron * speed_of_light * speed_of_light4);
}

double evaluateMcDonaldIntegral(const double& nu) {
	int curIndex = 0;
	if (nu < UvarovX[0]) {
		return 0;
	}
	if (nu > UvarovX[Napprox - 1]) {
		return 0;
	}
	for (int i = 1; i < Napprox; ++i) {
		if (nu < UvarovX[i]) {
			curIndex = i;
			break;
		}
	}

	//double result = (UvarovValue[curIndex]*(nu - UvarovX[curIndex - 1]) + UvarovValue[curIndex - 1]*(UvarovX[curIndex] - nu))/(UvarovX[curIndex] - UvarovX[curIndex - 1]);
	double result = UvarovValue[curIndex - 1] * exp(
		log(UvarovValue[curIndex] / UvarovValue[curIndex - 1]) * ((nu - UvarovX[curIndex - 1]) / (UvarovX[curIndex] - UvarovX[curIndex - 1])));
	if (result < 0) {
		printf("result < 0\n");
	}
	return result;
}

double evaluateMcDonaldFunction(const double& nu) {
	int curIndex = 0;
	if (nu < UvarovX[0]) {
		return 0;
	}
	if (nu > UvarovX[Napprox - 1]) {
		return 0;
	}
	for (int i = 1; i < Napprox; ++i) {
		if (nu < UvarovX[i]) {
			curIndex = i;
			break;
		}
	}

	//double result = (McDonaldValue[curIndex]*(nu - UvarovX[curIndex - 1]) + McDonaldValue[curIndex - 1]*(UvarovX[curIndex] - nu))/(UvarovX[curIndex] - UvarovX[curIndex - 1]);
	double result = McDonaldValue[curIndex - 1] * exp(
		log(McDonaldValue[curIndex] / McDonaldValue[curIndex - 1]) * ((nu - UvarovX[curIndex - 1]) / (UvarovX[curIndex] - UvarovX[curIndex - 1])));
	if (result < 0) {
		printf("result < 0\n");
	}
	return result;
}

void evaluateSpectrum(double* nu, double* Inu, double* Anu, int Nnu, double* Ee, double* Fe, int Np, double minEnergy, double maxEnergy, int startElectronIndex, double sinhi, double Bmean, double concentration, double localSize) {
	double minNu = 0.001 * criticalNu(minEnergy, sinhi, Bmean);
	double maxNu = 10 * criticalNu(maxEnergy, sinhi, Bmean);

	double temp = maxNu / minNu;

	double factor = power(maxNu / minNu, 1.0 / (Nnu - 1));

	nu[0] = minNu;
	Inu[0] = 0;
	Anu[0] = 0;
	for (int i = 1; i < Nnu; ++i) {
		nu[i] = nu[i - 1] * factor;
		Inu[i] = 0;
		Anu[i] = 0;
	}

	double coef = concentration * sqrt(3.0) * electron_charge * electron_charge * electron_charge / (massElectron * speed_of_light2);
	double coefAbsorb = concentration * 16 * pi * pi * electron_charge / (3 * sqrt(3.0) * Bmean * sinhi);


	for (int i = 0; i < Nnu; ++i) {
		printf("i = %d\n", i);
		for (int j = startElectronIndex; j < Np; ++j) {
			double nuc = criticalNu(Ee[j], sinhi, Bmean);
			double gamma = Ee[j] / (massElectron * speed_of_light2);
			double x = nu[i] / nuc;
			Inu[i] = Inu[i] + coef * Fe[j] * (Ee[j] - Ee[j - 1]) * Bmean * sinhi * evaluateMcDonaldIntegral(nu[i] / nuc);
			Anu[i] = Anu[i] + coefAbsorb * Fe[j] * (Ee[j] - Ee[j - 1]) * evaluateMcDonaldFunction(nu[i] / nuc) / (gamma * gamma * gamma * gamma * gamma);
		}
	}
	int absorbtionIndex = 0;
	for (int i = 0; i < Nnu; ++i) {
		if (Anu[i] * localSize < 1) {
			absorbtionIndex = i;
			break;
		}
	}

	for (int i = 0; i < Nnu; ++i) {
		Inu[i] = Inu[i] * (1 - exp(-Anu[i] * localSize)) / (Anu[i] * localSize);
	}
}

void findMaxNu(int& nuMaxIndex, double* Inu, int Nnu) {
	double Imax = 0;
	nuMaxIndex = 0;
	for(int i = 0; i < Nnu; ++i) {
		if(Inu[i] > Imax) {
			Imax = Inu[i];
			nuMaxIndex = i;
		}
	}
}

double findEmissivityAt(double* nu, double* Inu, double currentNu, int Nnu) {
	if(currentNu <= nu[0]) {
		return Inu[0];
	}
	if(currentNu >= nu[Nnu - 1]) {
		return Inu[Nnu - 1];
	}
	for(int i = 1; i < Nnu; ++i) {
		if(currentNu < nu[i]) {
			return Inu[i - 1] * exp(log(Inu[i] / Inu[i - 1]) * ((currentNu - nu[i-1]) / (nu[i] - nu[i - 1])));
		}
	}
	return 0;
}

void evaluateDoplerSpectrum(double* doplerInu, double* nu, double* Inu, double gamma0, int Nnu) {
	for(int i = 0; i < Nnu; ++i) {
		doplerInu[i] = 0;
	}
	double beta = sqrt(1.0 - 1.0/(gamma0*gamma0));
	int Nmu = 20;
	for(int j = 0; j < Nmu; ++j) {
		double mu = -1.0 + (2*j + 1.0)/Nmu;
		double D = 1.0/(gamma0*(1 - beta*mu));
		for(int i = 0; i < Nnu; ++i) {
			double tempNu = nu[i]/D;
			double tempI = findEmissivityAt(nu, Inu, tempNu, Nnu);
			doplerInu[i] = doplerInu[i] + D*D*tempI*(2.0/Nmu);
		}
	}
}

int main(int argc, char** argv) {

	int Np = 200;
	FILE* inputPe = fopen("../../tristan-mp-pitp/Pe.dat", "r");
	FILE* inputFe = fopen("../../tristan-mp-pitp/Fe.dat", "r");

	double* Pe = new double[Np];
	double* Fe = new double[Np];
	double* Ee = new double[Np];

	int Nnu = 100;
	double* nu = new double[Nnu];
	double* Inu = new double[Nnu];
	double* doplerInu = new double[Nnu];
	double* Anu = new double[Nnu];

	double sigma = 0.36;
	double gamma0 = 1.2;
	double v = speed_of_light * sqrt(1 - 1 / (gamma0 * gamma0));
	const int Npoints = 4;


	double B0 = 0.4;
	double n0 = 5;
	double L0 = 1E16;

	double L3 = 2.16E17;
	double n3 = 3;
	double B3 = 0.1;

	double B3min = 0.01;
	double B3max = 100;

	double size[Npoints];
	double B[Npoints];
	double n[Npoints];
	double time[Npoints];

	time[0] = 0;
	//B[0] = B0;
	//n[0] = n0;
	//size[0] = L0;

	B[3] = B3;
	n[3] = n3;
	size[3] = L3;

	time[1] = 2760000;
	time[2] = 5270400;
	time[3] = 10700000;

	for(int i = 2; i >= 0; --i) {
		size[i] = size[3] - v*(time[3] - time[i]);
		if(size[i] < 0) {
			printf("aaaa, size < 0!!!");
			exit(0);
		}
	}

	/*for (int i = 1; i < Npoints; ++i) {
		B[i] = B[0] * size[0] / size[i];
		n[i] = n[0] * size[0] * size[0] / (size[i] * size[i]);
	}*/

	for (int i = 0; i < Npoints-1; ++i) {
		B[i] = B[3] * size[3] / size[i];
		n[i] = n[3] * size[3] * size[3] / (size[i] * size[i]);
	}

	for (int i = 0; i < Np; ++i) {
		fscanf(inputPe, "%lf", &Pe[i]);
		Pe[i] = Pe[i] * massElectron * speed_of_light;
		Ee[i] = sqrt(Pe[i] * Pe[i] * speed_of_light2 + massElectron * massElectron * speed_of_light4);
		fscanf(inputFe, "%lf", &Fe[i]);
		Fe[i] = Fe[i] * Ee[i] / (Pe[i] * Pe[i] * Pe[i] * speed_of_light2);
	}

	/*Fe[0] = 1.0;
	for(int i = 1; i < Np; ++i) {
		Fe[i] = Fe[0]*power(Ee[0]/Ee[i],3);
	}*/

	fclose(inputPe);
	fclose(inputFe);

	double norm = 0;
	for (int i = 1; i < Np; ++i) {
		norm = norm + Fe[i] * (Ee[i] - Ee[i - 1]);
	}
	for (int i = 0; i < Np; ++i) {
		Fe[i] = Fe[i] / norm;
	}

	double minEnergy = 10 * massElectron * speed_of_light2;
	//double minEnergy = Ee[0];
	double maxEnergy = Ee[Np - 1];

	double meanE = 200 * massElectron * speed_of_light2;

	int startElectronIndex = 0;
	for (int i = 0; i < Np; ++i) {
		if (Ee[i] > minEnergy) {
			startElectronIndex = i;
			break;
		}
	}

	double hi = pi / 4;
	double sinhi = sin(hi);

	bool converges = false;
	double leftB = B3min;
	double rightB = B3max;
	double localConcentration = n[3];
	double localSize = size[3];
	double localB = (leftB + rightB)/2;
	evaluateSpectrum(nu, Inu, Anu, Nnu, Ee, Fe, Np, minEnergy, maxEnergy, startElectronIndex, sinhi, localB, localConcentration, localSize);
	double nuMax = 0;
	int nuMaxIndex = 0;
	findMaxNu(nuMaxIndex, Inu, Nnu);
	nuMax = nu[nuMaxIndex];
	int iterations = 0;
	double factor = 1;
	while(!converges) {
		iterations++;
		if(nuMax > augmaxx*1E9) {
			rightB = localB;
			localB = (leftB + rightB)/2;
			evaluateSpectrum(nu, Inu, Anu, Nnu, Ee, Fe, Np, minEnergy, maxEnergy, startElectronIndex, sinhi, localB, localConcentration, localSize);
			findMaxNu(nuMaxIndex, Inu, Nnu);
		} else {
			leftB = localB;
			localB = (leftB + rightB)/2;
			evaluateSpectrum(nu, Inu, Anu, Nnu, Ee, Fe, Np, minEnergy, maxEnergy, startElectronIndex, sinhi, localB, localConcentration, localSize);
			findMaxNu(nuMaxIndex, Inu, Nnu);
		}
		nuMax = nu[nuMaxIndex];
		if((rightB - leftB) < 0.0001) {
			converges = true;
		}
	}
	factor = augmaxy/Inu[nuMaxIndex];

	printf("B august = %g\n", localB);
	B[3] = localB;
	for (int i = 0; i < Npoints-1; ++i) {
		B[i] = B[3] * size[3] / size[i];
	}


	for (int k = 0; k < Npoints; ++k) {

		double Bmean = B[k];

		localConcentration = n[k];
		localSize = size[k];


		evaluateSpectrum(nu, Inu, Anu, Nnu, Ee, Fe, Np, minEnergy, maxEnergy, startElectronIndex, sinhi, Bmean, localConcentration, localSize);

		evaluateDoplerSpectrum(doplerInu, nu, Inu, gamma0, Nnu);

		double totalFlux = 0;
		for (int i = 1; i < Nnu; ++i) {
			totalFlux += Inu[i] * (nu[i] - nu[i - 1]) * localSize * localSize * localSize;
		}


		double nuc = criticalNu(minEnergy, sinhi, Bmean);
		std::string fileName = "../../tristan-mp-pitp/radiation";
		char* number = new char[100];
		itoa(k, number, 10);
		std::string fileNumber = std::string(number);
		FILE* output = fopen((fileName + fileNumber + ".dat").c_str(), "w");
		delete[] number;
		for (int i = 0; i < Nnu; ++i) {
			double sizeFactor = localSize*localSize*localSize/(size[3]*size[3]*size[3]);
			double totalInu = Inu[i]*sizeFactor;
			fprintf(output, "%g %g %g %g %g %g %g %g\n", nu[i]/1E9, Inu[i], Anu[i] * localSize, totalInu, Inu[i]*factor, totalInu*factor, doplerInu[i]*factor, doplerInu[i]*sizeFactor*factor);
		}

		fclose(output);
	}

	delete[] Pe;
	delete[] Ee;
	delete[] Fe;

	delete[] nu;
	delete[] Inu;
	delete[] doplerInu;
	delete[] Anu;
}