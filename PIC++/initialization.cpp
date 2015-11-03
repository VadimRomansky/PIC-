#include "stdlib.h"
#include "stdio.h"
#include <cmath>
#include <omp.h>

#include "simulation.h"
#include "specialmath.h"
#include "util.h"
#include "output.h"
#include "constants.h"
#include "matrix3d.h"
#include "random.h"

Simulation::Simulation(){
	newlyStarted = false;

	maxEfield = Vector3d(0, 0, 0);
	maxBfield = Vector3d(0, 0, 0);

	Kronecker = Matrix3d(1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0);

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			for (int k = 0; k < 3; ++k) {
				LeviCivita[i][j][k] = 0;
			}
		}
	}
	LeviCivita[0][1][2] = 1.0;
	LeviCivita[0][2][1] = -1.0;
	LeviCivita[1][0][2] = -1.0;
	LeviCivita[1][2][0] = 1.0;
	LeviCivita[2][0][1] = 1.0;
	LeviCivita[2][1][0] = -1.0;
}

Simulation::Simulation(int xn, int yn, int zn, double xsizev, double ysizev, double zsizev, double temp, double rho, double Vx, double Vy, double Vz, double Ex, double Ey, double Ez, double Bx, double By, double Bz, int maxIterations, double maxTimeV, int particlesPerBinV) {
	debugMode = true;
	newlyStarted = true;
	solverType = IMPLICIT;
	//solverType = EXPLICIT;
	boundaryConditionType = PERIODIC;
	//boundaryConditionType = SUPER_CONDUCTOR_LEFT;
	maxwellEquationMatrixSize = 3;

	currentIteration = 0;
	time = 0;
	particlesNumber = 0;

	particleEnergy = 0;
	electricFieldEnergy = 0;
	magneticFieldEnergy = 0;

	momentum = Vector3d(0, 0, 0);


	theta = initialTheta;
	eta = 0.5;

	xnumber = xn;
	ynumber = yn;
	znumber = zn;

	xsize = xsizev;
	ysize = ysizev;
	zsize = zsizev;

	temperature = temp;
	density = rho;

	maxIteration = maxIterations;
	maxTime = maxTimeV;

	particlesPerBin = particlesPerBinV;

	extJ = 0;

	V0 = Vector3d(Vx, Vy, Vz);

	B0 = Vector3d(Bx, By, Bz);
	E0 = Vector3d(Ex, Ey, Ez);

	double concentration = density / (massProton + massElectron);

	plasma_period = sqrt(massElectron / (4 * pi * concentration * sqr(electron_charge))) * (2 * pi);
	double thermal_momentum;
	if (kBoltzman * temperature > massElectron * speed_of_light * speed_of_light) {
		thermal_momentum = kBoltzman * temperature / speed_of_light;
	} else {
		thermal_momentum = sqrt(2 * massElectron * kBoltzman * temperature);
	}
	thermal_momentum += V0.norm()*massElectron;
	gyroradius = thermal_momentum * speed_of_light / (electron_charge * B0.norm());
	if(B0.norm() <= 0){
		gyroradius = 1.0;
	}

	//plasma_period = 1.0;
	//gyroradius = 1.0;

	//gyroradius = xsize;

	

	E0 = E0 * (plasma_period * sqrt(gyroradius));
	B0 = B0 * (plasma_period * sqrt(gyroradius));
	V0 = V0 * plasma_period/gyroradius;

	fieldScale = max2(B0.norm(), E0.norm());
	if(fieldScale <= 0){
		fieldScale = 1.0;
	}
	fieldScale = 1.0;

	E0 = E0/fieldScale;
	B0 = B0/fieldScale;

	rescaleConstants();

	density = density * cube(gyroradius);

	xsize /= gyroradius;
	ysize /= gyroradius;
	zsize /= gyroradius;
	printf("xsize/gyroradius = %lf\n", xsize);

	maxEfield = Vector3d(0, 0, 0);
	maxBfield = Vector3d(0, 0, 0);

	Kronecker = Matrix3d(1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0);

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			for (int k = 0; k < 3; ++k) {
				LeviCivita[i][j][k] = 0;
			}
		}
	}
	LeviCivita[0][1][2] = 1.0;
	LeviCivita[0][2][1] = -1.0;
	LeviCivita[1][0][2] = -1.0;
	LeviCivita[1][2][0] = 1.0;
	LeviCivita[2][0][1] = 1.0;
	LeviCivita[2][1][0] = -1.0;
}

Simulation::~Simulation() {

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				delete[] maxwellEquationMatrix[i][j][k];
				delete[] maxwellEquationRightPart[i][j][k];
			}
			delete[] maxwellEquationMatrix[i][j];
			delete[] maxwellEquationRightPart[i][j];
		}
		delete[] maxwellEquationMatrix[i];
		delete[] maxwellEquationRightPart[i];
	}
	delete[] maxwellEquationMatrix;
	delete[] maxwellEquationRightPart;

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				delete[] divergenceCleaningField[i][j][k];
				delete[] divergenceCleaningPotential[i][j][k];
				delete[] divergenceCleanUpMatrix[i][j][k];
				delete[] divergenceCleanUpRightPart[i][j][k];
			}
			delete[] divergenceCleaningField[i][j];
			delete[] divergenceCleaningPotential[i][j];
			delete[] divergenceCleanUpMatrix[i][j];
			delete[] divergenceCleanUpRightPart[i][j];
		}
		delete[] divergenceCleaningField[i];
		delete[] divergenceCleaningPotential[i];
		delete[] divergenceCleanUpMatrix[i];
		delete[] divergenceCleanUpRightPart[i];
	}
	delete[] divergenceCleaningField;
	delete[] divergenceCleaningPotential;
	delete[] divergenceCleanUpMatrix;
	delete[] divergenceCleanUpRightPart;

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber + 1; ++j){
			delete[] Efield[i][j];
			delete[] newEfield[i][j];
			delete[] tempEfield[i][j];
			delete[] explicitEfield[i][j];
			delete[] rotB[i][j];
			delete[] Ederivative[i][j];
	
			delete[] electricFlux[i][j];
			delete[] externalElectricFlux[i][j];
			delete[] divPressureTensor[i][j];
			delete[] dielectricTensor[i][j];
		}
		delete[] Efield[i];
		delete[] newEfield[i];
		delete[] tempEfield[i];
		delete[] explicitEfield[i];
		delete[] rotB[i];
		delete[] Ederivative[i];

		delete[] electricFlux[i];
		delete[] externalElectricFlux[i];
		delete[] divPressureTensor[i];
		delete[] dielectricTensor[i];
	}

	delete[] Efield;
	delete[] newEfield;
	delete[] tempEfield;
	delete[] explicitEfield;
	delete[] rotB;
	delete[] Ederivative;

	delete[] electricFlux;
	delete[] externalElectricFlux;
	delete[] divPressureTensor;
	delete[] dielectricTensor;

	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber; ++j){
			delete[] Bfield[i][j];
			delete[] newBfield[i][j];

			delete[] electronConcentration[i][j];
			delete[] protonConcentration[i][j];
			delete[] velocityBulk[i][j];
			delete[] velocityBulkElectron[i][j];
			delete[] chargeDensity[i][j];
			delete[] pressureTensor[i][j];
			delete[] electricDensity[i][j];
		}
		delete[] Bfield[i];
		delete[] newBfield[i];

		delete[] electronConcentration[i];
		delete[] protonConcentration[i];
		delete[] velocityBulk[i];
		delete[] velocityBulkElectron[i];
		delete[] chargeDensity[i];
		delete[] pressureTensor[i];
		delete[] electricDensity[i];
	}

	delete[] Bfield;
	delete[] newBfield;

	delete[] electronConcentration;
	delete[] protonConcentration;
	delete[] velocityBulk;
	delete[] velocityBulkElectron;
	delete[] chargeDensity;
	delete[] pressureTensor;
	delete[] electricDensity;

	delete[] xgrid;
	delete[] middleXgrid;
	delete[] ygrid;
	delete[] middleYgrid;
	delete[] zgrid;
	delete[] middleZgrid;
}

void Simulation::rescaleConstants(){
	kBoltzman_normalized = kBoltzman * plasma_period * plasma_period / (gyroradius * gyroradius);
	speed_of_light_normalized = speed_of_light * plasma_period / gyroradius;
	speed_of_light_normalized_sqr = speed_of_light_normalized * speed_of_light_normalized;
	electron_charge_normalized = electron_charge * (plasma_period / sqrt(cube(gyroradius)));
}

void Simulation::initialize() {
	printf("initialization\n");

	deltaX = xsize / (xnumber);
	deltaY = ysize / (ynumber);
	deltaZ = zsize / (znumber);

	deltaX2 = deltaX * deltaX;
	deltaY2 = deltaY * deltaY;
	deltaZ2 = deltaZ * deltaZ;

	for (int i = 0; i <= xnumber; ++i) {
		xgrid[i] = xsize + i * deltaX;
	}
	xgrid[xnumber] = 2*xsize;

	for (int j = 0; j <= ynumber; ++j) {
		ygrid[j] = ysize +j * deltaY;
	}
	ygrid[ynumber] = 2*ysize;

	for (int k = 0; k <= znumber; ++k) {
		zgrid[k] = zsize + k * deltaZ;
	}
	zgrid[znumber] = 2*zsize;

	for (int i = 0; i < xnumber; ++i) {
		middleXgrid[i] = (xgrid[i] + xgrid[i + 1]) / 2;
	}

	for (int j = 0; j < ynumber; ++j) {
		middleYgrid[j] = (ygrid[j] + ygrid[j + 1]) / 2;
	}

	for (int k = 0; k < znumber; ++k) {
		middleZgrid[k] = (zgrid[k] + zgrid[k + 1]) / 2;
	}

	for (int i = 0; i < xnumber + 1; ++i) {
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k] = E0;
				newEfield[i][j][k] = Efield[i][j][k];
				tempEfield[i][j][k] = Efield[i][j][k];
				explicitEfield[i][j][k] = Efield[i][j][k];
				rotB[i][j][k] = Vector3d(0, 0, 0);
				Ederivative[i][j][k] = Vector3d(0, 0, 0);
				externalElectricFlux[i][j][k] = Vector3d(0, 0, 0);
			}
		}
	}

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k] = B0;
				//Bfield[i][j][k].x = B0.x;
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}

	checkDebyeParameter();

	double concentration = density / (massProton + massElectron);

	omegaPlasmaProton = sqrt(4 * pi * concentration * electron_charge_normalized * electron_charge_normalized / massProton);
	omegaPlasmaElectron = sqrt(4 * pi * concentration * electron_charge_normalized * electron_charge_normalized / massElectron);

	informationFile = fopen("./output/information.dat", "a");
	if(omegaPlasmaElectron*xsize/speed_of_light_normalized < 5){
		printf("omegaPlasmaElectron*xsize/speed_of_light_normalized < 5\n");
		fprintf(informationFile, "omegaPlasmaElectron*xsize/speed_of_light_normalized < 5\n");
	}
	printf("omegaPlasmaElectron*xsize/speed_of_light_normalized = %g\n", omegaPlasmaElectron*xsize/speed_of_light_normalized);
	fprintf(informationFile, "omegaPlasmaElectron*xsize/speed_of_light_normalized = %g\n", omegaPlasmaElectron*xsize/speed_of_light_normalized);

	if(omegaPlasmaElectron*deltaX/speed_of_light_normalized > 1){
		printf("omegaPlasmaElectron*deltaX/speed_of_light_normalized > 1\n");
		fprintf(informationFile, "omegaPlasmaElectron*deltaX/speed_of_light_normalized > 1\n");
	}
	printf("omegaPlasmaElectron*deltaX/speed_of_light_normalized = %g\n", omegaPlasmaElectron*deltaX/speed_of_light_normalized);
	fprintf(informationFile, "omegaPlasmaElectron*deltaX/speed_of_light_normalized = %g\n", omegaPlasmaElectron*deltaX/speed_of_light_normalized);
	fclose(informationFile);

	checkDebyeParameter();
	checkGyroRadius();

	omegaGyroProton = electron_charge * B0.norm()*fieldScale / (massProton * speed_of_light);
	omegaGyroProton = electron_charge * B0.norm()*fieldScale / (massElectron * speed_of_light);

}

void Simulation::initializeSimpleElectroMagneticWave() {
	E0 = Vector3d(0, 0, 0);
	B0 = Vector3d(0, 0, 0);
	for (int i = 0; i < xnumber; ++i) {
		for( int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k] = Vector3d(0, 0, 0);
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}
	double kw = 2 * pi / xsize;
	double E = 1E-5;

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Efield[i][j][k].x = 0;
				Efield[i][j][k].y = E * sin(kw * xgrid[i]);
				Efield[i][j][k].z = 0;
				tempEfield[i][j][k] = Efield[i][j][k];
				newEfield[i][j][k] = tempEfield[i][j][k];
				explicitEfield[i][j][k] = Efield[i][j][k];
			}
		}
	}

	for(int k = 0; k < znumber; ++k){
		for(int j = 0; j < ynumber; ++j){
			Efield[xnumber][j][k] = Efield[0][j][k];
			tempEfield[xnumber][j][k] = Efield[0][j][k];
			newEfield[xnumber][j][k] = Efield[0][j][k];
			explicitEfield[xnumber][j][k] = explicitEfield[0][j][k];
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber; ++j){
			Efield[i][j][znumber] = Efield[i][j][0];
			tempEfield[i][j][znumber] = Efield[i][j][0];
			newEfield[i][j][znumber] = Efield[i][j][0];
			explicitEfield[i][j][znumber] = explicitEfield[i][j][0];
		}
	}

	for(int k = 0; k < znumber + 1; ++k){
		for(int i = 0; i < xnumber + 1; ++i){
			Efield[i][ynumber][k] = Efield[i][0][k];
			tempEfield[i][ynumber][k] = Efield[i][0][k];
			newEfield[i][ynumber][k] = Efield[i][0][k];
			explicitEfield[i][ynumber][k] = explicitEfield[i][0][k];
		}
	}

	for (int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k].z = E * sin(kw * middleXgrid[i]);
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}

	double t = 2 * pi / (kw * speed_of_light_normalized);
}

void Simulation::initializeAlfvenWave(int wavesCount, double amplitudeRelation) {
	printf("initialization alfven wave\n");
	E0 = Vector3d(0, 0, 0);

	informationFile = fopen("./output/information.dat", "a");

	double alfvenV = B0.norm()*fieldScale / sqrt(4 * pi * density);
	if (alfvenV > speed_of_light_normalized) {
		printf("alfven velocity > c\n");
		fprintf(informationFile, "alfven velocity > c\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "alfvenV/c = %15.10g > 1\n", alfvenV/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	fprintf(informationFile, "alfven V = %lf\n", alfvenV*gyroradius/plasma_period);
	fprintf(informationFile, "alfven V/c = %lf\n", alfvenV/speed_of_light_normalized);
	printf("alfven V = %lf\n", alfvenV*gyroradius/plasma_period);
	printf("alfven V/c = %lf\n", alfvenV/speed_of_light_normalized);

	double kw = wavesCount * 2 * pi / xsize;

	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;

	omegaPlasmaProton = sqrt(4*pi*concentration*electron_charge_normalized*electron_charge_normalized/massProton);
	omegaPlasmaElectron = sqrt(4*pi*concentration*electron_charge_normalized*electron_charge_normalized/massElectron);
	omegaGyroProton = B0.norm()*fieldScale*electron_charge_normalized/(massProton*speed_of_light_normalized);
	omegaGyroElectron = B0.norm()*fieldScale*electron_charge_normalized/(massElectron*speed_of_light_normalized);

	if(omegaGyroProton < 5*speed_of_light_normalized*kw){
		printf("omegaGyroProton < 5*k*c\n");
		fprintf(informationFile, "omegaGyroProton < 5*k*c\n");
		//fclose(informationFile);
		//exit(0);
	}
	printf("omegaGyroProton/kc = %g\n", omegaGyroProton/(kw*speed_of_light_normalized));
	fprintf(informationFile, "omegaGyroProton/kc = %g\n", omegaGyroProton/(kw*speed_of_light_normalized));

	if(omegaPlasmaProton < 5*omegaGyroProton){
		printf("omegaPlasmaProton < 5*omegaGyroProton\n");
		fprintf(informationFile, "omegaPlasmaProton < 5*omegaGyroProton\n");
		//fclose(informationFile);
		//exit(0);
	}
	printf("omegaPlasmaProton/omegaGyroProton = %g\n", omegaPlasmaProton/omegaGyroProton);
	fprintf(informationFile, "omegaPlasmaProton/omegaGyroProton = %g\n", omegaPlasmaProton/omegaGyroProton);

	//w = q*kw*B/mP * 0.5*(sqrt(d)+-b)/a
	double b = speed_of_light_normalized*kw*(massProton - massElectron)/massProton;
	double discriminant = speed_of_light_normalized_sqr*kw*kw*sqr(massProton + massElectron) + 16*pi*concentration*sqr(electron_charge_normalized)*(massProton + massElectron)/sqr(massProton);
	double a = (kw*kw*speed_of_light_normalized_sqr*massProton*massElectron + 4*pi*concentration*sqr(electron_charge_normalized)*(massProton + massElectron))/sqr(massProton);

	if(discriminant < 0){
		printf("discriminant < 0\n");
		fprintf(informationFile, "discriminant < 0\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "discriminant = %15.10g\n", discriminant);
		fclose(errorLogFile);
		exit(0);
	}

	double fakeOmega = (kw*electron_charge_normalized*B0.norm()*fieldScale/massProton)*(sqrt(discriminant) - b)/(2.0*a);

	//a4*x^4 + a3*x^3 + a2*x^2 + a1*x + a0 = 0

	double a4 = sqr(speed_of_light_normalized_sqr*massProton*massElectron);
	a4 = a4*sqr(sqr(sqr(fakeOmega)));
	double a3 = -2*cube(speed_of_light_normalized_sqr)*sqr(kw*massElectron*massProton)
		- 8*pi*concentration*sqr(speed_of_light_normalized_sqr*electron_charge_normalized)*massElectron*massProton*(massElectron + massProton)
		- sqr(B0.norm()*fieldScale*speed_of_light_normalized*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron));
	a3 = a3*cube(sqr(fakeOmega));
	double a2 = sqr(sqr(speed_of_light_normalized_sqr*kw)*massProton*massElectron)
		+ 8*pi*cube(speed_of_light_normalized_sqr)*concentration*sqr(kw*electron_charge_normalized)*massProton*massElectron*(massProton + massElectron)
		+ 16*sqr(pi*speed_of_light_normalized_sqr*concentration*sqr(electron_charge_normalized)*(massProton + massElectron))
		+ 2*sqr(B0.norm()*fieldScale*speed_of_light_normalized_sqr*kw*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron))
		+ 8*pi*concentration*sqr(B0.norm()*fieldScale*speed_of_light_normalized*sqr(electron_charge_normalized))*(massProton + massElectron)
		+ sqr(sqr(B0.norm()*fieldScale*electron_charge_normalized));
	a2 = a2*sqr(sqr(fakeOmega));
	double a1 = - sqr(B0.norm()*fieldScale*cube(speed_of_light_normalized)*kw*kw*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron))
		- 8*pi*concentration*sqr(B0.norm()*fieldScale*speed_of_light_normalized_sqr*kw*sqr(electron_charge_normalized))*(massProton + massElectron)
		- 2*sqr(speed_of_light_normalized*kw*sqr(B0.norm()*fieldScale*electron_charge_normalized));
	a1 = a1*sqr(fakeOmega);
	double a0 = sqr(sqr(B0.norm()*fieldScale*speed_of_light_normalized*kw*electron_charge_normalized));

	a4 = a4/a0;
	a3 = a3/a0;
	a2 = a2/a0;
	a1 = a1/a0;
	a0 = 1.0;

	printf("a4 = %g\n", a4);
	fprintf(informationFile, "a4 = %g\n", a4);
	printf("a3 = %g\n", a3);
	fprintf(informationFile, "a3 = %g\n", a3);
	printf("a2 = %g\n", a2);
	fprintf(informationFile, "a2 = %g\n", a2);
	printf("a1 = %g\n", a1);
	fprintf(informationFile, "a1 = %g\n", a1);
	printf("a0 = %g\n", a0);
	fprintf(informationFile, "a0 = %g\n", a0);

	double fakeOmega1 = kw*alfvenV;
	printf("fakeOmega = %g\n", fakeOmega1/plasma_period);
	fprintf(informationFile, "fakeOmega = %g\n", fakeOmega1/plasma_period);
	double realOmega2 = solve4orderEquation(a4, a3, a2, a1, a0, 1.0);
	if(realOmega2 < 0){
		printf("omega^2 < 0\n");
		fprintf(informationFile, "omega^2 < 0\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "omega^2 = %15.10g > 1\n", realOmega2);
		fclose(errorLogFile);
		exit(0);
	}

	double error = (((a4*realOmega2 + a3)*realOmega2 + a2)*realOmega2 + a1)*realOmega2 + a0;
	printf("error = %15.10g\n", error);
	fprintf(informationFile, "error = %15.10g\n", error);
	//double 
	omega = sqrt(realOmega2)*fakeOmega;
	if(omega < 0){
		omega = - omega;
	}

	if(omega > speed_of_light_normalized*kw/5.0){
		printf("omega > k*c/5\n");
		fprintf(informationFile, "omega > k*c/5\n");
		printf("omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
		fprintf(informationFile, "omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
		//fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "omega/kc = %15.10g > 0.2\n", omega/(kw*speed_of_light_normalized));
		fclose(errorLogFile);
		//exit(0);
	}
	printf("omega = %g\n", omega/plasma_period);
	fprintf(informationFile, "omega = %g\n", omega/plasma_period);

	printf("omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
	fprintf(informationFile, "omega/kc = %g\n", omega/(kw*speed_of_light_normalized));

	if(fabs(omega) > omegaGyroProton/2){
		printf("omega > omegaGyroProton/2\n");
		fprintf(informationFile, "omega > omegaGyroProton/2\n");
	}
	printf("omega/omegaGyroProton = %g\n", omega/omegaGyroProton);
	fprintf(informationFile, "omega/omegaGyroProton = %g\n", omega/omegaGyroProton);
	fclose(informationFile);

	checkFrequency(omega);

	informationFile = fopen("./output/information.dat", "a");
	//checkCollisionTime(omega);
	//checkMagneticReynolds(alfvenV);
	//checkDissipation(kw, alfvenV);

	double epsilonAmplitude = amplitudeRelation;

	double alfvenVReal = omega/kw;

	//double 
	Bzamplitude = B0.norm() * epsilonAmplitude;

	double Omegae = omegaGyroElectron;
	double Omegae2 = Omegae*Omegae;
	double Omegap = omegaGyroProton;
	double Omegap2 = Omegap*Omegap;
	double omegae = omegaPlasmaElectron;
	double omegae2 = omegae*omegae;
	double omegap = omegaPlasmaProton;
	double omegap2 = omegap*omegap;

	double kc = kw*speed_of_light_normalized;
	double kc2 = kc*kc;

	double denominator = omega*omega - Omegae2 - (omegae2*omega*omega/(omega*omega - kc2));

	//double 
	VzamplitudeProton = -((1.0/(4*pi*concentration*electron_charge_normalized))*(kc + ((omegae2 + omegap2 - omega*omega)/kc) + (omegae2*Omegae2/(kc*denominator)))/((Omegae*omegae2*omega/((kc2 - omega*omega)*denominator)) + (Omegap/omega)))*Bzamplitude*fieldScale;
	//double 
	VzamplitudeElectron = (((electron_charge_normalized*omega*Omegae)/(massElectron*kc))*Bzamplitude*fieldScale + (omegae2*omega*omega/(kc2 - omega*omega))*VzamplitudeProton)/denominator;

	//double 
	Byamplitude = (4*pi*concentration*electron_charge_normalized/((omega*omega/kc) - kc))*(VzamplitudeElectron - VzamplitudeProton)/fieldScale;

	//double 
	VyamplitudeProton = -(Omegap/omega)*VzamplitudeProton - (electron_charge_normalized/(massProton*kc))*Bzamplitude*fieldScale;
	//double 
	VyamplitudeElectron = (Omegae/omega)*VzamplitudeElectron + (electron_charge_normalized/(massElectron*kc))*Bzamplitude*fieldScale;

	//double 
	Eyamplitude = (omega/kc)*Bzamplitude;
	//double 
	Ezamplitude = -(omega/kc)*Byamplitude;

	double xshift = 0.0;

	//Eyamplitude = 0.0;
	//VzamplitudeElectron = 0.0;
	//VzamplitudeProton = 0.0;
	//Byamplitude = 0.0;

	for (int i = 0; i < xnumber + 1; ++i) {
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k].x = 0;
				Efield[i][j][k].y = Eyamplitude * cos(kw * xgrid[i] - kw*xshift);
				Efield[i][j][k].z = Ezamplitude * sin(kw * xgrid[i] - kw*xshift);
				explicitEfield[i][j][k] = Efield[i][j][k];
				tempEfield[i][j][k] = Efield[i][j][k];
				newEfield[i][j][k] = Efield[i][j][k];
			}
		}
	}

	for(int k = 0; k < znumber; ++k){
		for(int j = 0; j < ynumber; ++j){
			Efield[xnumber][j][k] = Efield[0][j][k];
			tempEfield[xnumber][j][k] = Efield[0][j][k];
			newEfield[xnumber][j][k] = Efield[0][j][k];
			explicitEfield[xnumber][j][k] = explicitEfield[0][j][k];
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber; ++j){
			Efield[i][j][znumber] = Efield[i][j][0];
			tempEfield[i][j][znumber] = Efield[i][j][0];
			newEfield[i][j][znumber] = Efield[i][j][0];
			explicitEfield[i][j][znumber] = explicitEfield[i][j][0];
		}
	}

	for(int k = 0; k < znumber + 1; ++k){
		for(int i = 0; i < xnumber + 1; ++i){
			Efield[i][ynumber][k] = Efield[i][0][k];
			tempEfield[i][ynumber][k] = Efield[i][0][k];
			newEfield[i][ynumber][k] = Efield[i][0][k];
			explicitEfield[i][ynumber][k] = explicitEfield[i][0][k];
		}
	}

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k].x = B0.norm();
				Bfield[i][j][k].y = Byamplitude * sin(kw * middleXgrid[i] - kw*xshift);
				Bfield[i][j][k].z = Bzamplitude * cos(kw * middleXgrid[i] - kw*xshift);
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}

	if(fabs(VzamplitudeProton) > speed_of_light_normalized){
		printf("VzamplitudeProton > speed_of_light_normalized\n");
		fprintf(informationFile, "VzamplitudeProton > speed_of_light_normalized\n");
		printf("VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
		fprintf(informationFile, "VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VzamplitudeProton/c = %15.10g > 1\n", VzamplitudeProton/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	printf("VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
	fprintf(informationFile, "VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);

	if(fabs(VzamplitudeElectron) > speed_of_light_normalized){
		printf("VzamplitudeElectron > speed_of_light_normalized\n");
		fprintf(informationFile, "VzamplitudeElectron > speed_of_light_normalized\n");
		printf("VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
		fprintf(informationFile, "VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VzamplitudeElectron/c = %15.10g > 1\n", VzamplitudeElectron/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	printf("VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
	fprintf(informationFile, "VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);

	if(fabs(VyamplitudeProton) > speed_of_light_normalized){
		printf("VyamplitudeProton > speed_of_light_normalized\n");
		fprintf(informationFile, "VyamplitudeProton > speed_of_light_normalized\n");
		printf("VyamplitudeProton/c = %g\n", VyamplitudeProton/speed_of_light_normalized);
		fprintf(informationFile, "VyamplitudeProton/c = %g\n", VyamplitudeProton/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VyamplitudeProton/c = %15.10g > 1\n", VyamplitudeProton/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}

	if(fabs(VyamplitudeElectron) > speed_of_light_normalized){
		printf("VyamplitudeElectron > speed_of_light_normalized\n");
		fprintf(informationFile, "VyamplitudeElectron > speed_of_light_normalized\n");
		printf("VyamplitudeElectron/c = %g\n", VyamplitudeElectron/speed_of_light_normalized);
		fprintf(informationFile, "VyamplitudeElectron/c = %g\n", VyamplitudeElectron/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VyamplitudeElectron/c = %15.10g > 1\n", VyamplitudeElectron/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}

	//k > 0, w > 0, kx-wt, Bz > 0, Vz < 0, Vyp > 0, Vye < 0

	for (int pcount = 0; pcount < particles.size(); ++pcount) {
		Particle* particle = particles[pcount];
		Vector3d velocity = particle->velocity(speed_of_light_normalized);
		int xn = (particle->coordinates.x - xgrid[0])/deltaX;
		double rightWeight = (particle->coordinates.x - xgrid[xn])/deltaX;
		double leftWeight = (xgrid[xn+1] - particle->coordinates.x)/deltaX;
		if (particle->type == PROTON) {
			velocity = (Vector3d(0, 0, 1) * (VzamplitudeProton) * cos(kw * particle->coordinates.x - kw*xshift) + Vector3d(0, 1, 0) * VyamplitudeProton * sin(kw * particle->coordinates.x - kw*xshift));
			//velocity = Vector3d(0, 0, 1) * (VzamplitudeProton) * (leftWeight*(cos(kw * (xgrid[xn] - xshift)) + rightWeight*cos(kw*(xgrid[xn+1] - xshift)))) + Vector3d(0, 1, 0) * VyamplitudeProton * (leftWeight*(sin(kw * (xgrid[xn] - xshift)) + rightWeight*sin(kw*(xgrid[xn+1] - xshift))));
		}
		if (particle->type == ELECTRON) {
			velocity = (Vector3d(0, 0, 1) * (VzamplitudeElectron) * cos(kw * particle->coordinates.x - kw*xshift) + Vector3d(0, 1, 0) * VyamplitudeElectron * sin(kw * particle->coordinates.x - kw*xshift));
			//velocity = Vector3d(0, 0, 1) * (VzamplitudeElectron) * (leftWeight*(cos(kw * (xgrid[xn] - xshift)) + rightWeight*cos(kw*(xgrid[xn+1] - xshift)))) + Vector3d(0, 1, 0) * VyamplitudeElectron * (leftWeight*(sin(kw * (xgrid[xn] - xshift)) + rightWeight*sin(kw*(xgrid[xn+1] - xshift))));
		}
		double beta = velocity.norm() / speed_of_light_normalized;
		particle->addVelocity(velocity, speed_of_light_normalized);
	}

	updateDeltaT();

	printf("dt/Talfven = %g\n", deltaT*omega/(2*pi));
	printf("dt = %g\n", deltaT*plasma_period);
	fprintf(informationFile, "dt/Talfven = %g\n", deltaT*omega/(2*pi));
	fprintf(informationFile, "dt = %g\n", deltaT*plasma_period);

	double Vthermal = sqrt(2*kBoltzman_normalized*temperature/massElectron);
	double thermalFlux = Vthermal*concentration*electron_charge_normalized/sqrt(1.0*particlesPerBin);
	double alfvenFlux = (VyamplitudeProton - VyamplitudeElectron)*concentration*electron_charge_normalized;
	if(thermalFlux > alfvenFlux/2){
		printf("thermalFlux > alfvenFlux/2\n");
		fprintf(informationFile, "thermalFlux > alfvenFlux/2\n");
	}
	printf("alfvenFlux/thermalFlux = %g\n", alfvenFlux/thermalFlux);
	fprintf(informationFile, "alfvenFlux/thermalFlux = %g\n", alfvenFlux/thermalFlux);
	double minDeltaT = deltaX/Vthermal;
	if(minDeltaT > deltaT){
		printf("deltaT < dx/Vthermal\n");
		fprintf(informationFile,  "deltaT < dx/Vthermal\n");

		//printf("deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
		//fprintf(informationFile, "deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
		
		//fclose(informationFile);
		//exit(0);
	}
	printf("deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
	fprintf(informationFile, "deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
	fprintf(informationFile, "\n");

	fprintf(informationFile, "Bz amplitude = %g\n", Bzamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "By amplitude = %g\n", Byamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "Vz amplitude p = %g\n", VzamplitudeProton*gyroradius/plasma_period);
	fprintf(informationFile, "Vz amplitude e = %g\n", VzamplitudeElectron*gyroradius/plasma_period);
	fprintf(informationFile, "Vy amplitude p = %g\n", VyamplitudeProton*gyroradius/plasma_period);
	fprintf(informationFile, "Vy amplitude e = %g\n", VyamplitudeElectron*gyroradius/plasma_period);
	fprintf(informationFile, "Ey amplitude = %g\n", Eyamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "Ez amplitude = %g\n", Ezamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "By/Ez = %g\n", Byamplitude/Ezamplitude);
	fprintf(informationFile, "Bz/Ey = %g\n", Bzamplitude/Eyamplitude);
	fprintf(informationFile, "4*pi*Jy amplitude = %g\n", 4*pi*concentration*electron_charge_normalized*(VyamplitudeProton - VyamplitudeElectron)/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBy amplitude = %g\n", speed_of_light_normalized*kw*Bzamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "4*pi*Jz amplitude = %g\n", 4*pi*concentration*electron_charge_normalized*(VzamplitudeProton - VzamplitudeElectron)/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBz amplitude = %g\n", speed_of_light_normalized*kw*Byamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative By amplitude = %g\n", -omega*Byamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "-c*rotEy = %g\n", speed_of_light_normalized*kw*Ezamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Bz amplitude = %g\n", omega*Bzamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "-c*rotEz = %g\n", speed_of_light_normalized*kw*Eyamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Ey amplitude = %g\n", omega*Eyamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBy - 4*pi*Jy = %g\n", (speed_of_light_normalized*kw*Bzamplitude*fieldScale - 4*pi*concentration*electron_charge_normalized*(VyamplitudeProton - VyamplitudeElectron))/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Ez amplitude = %g\n", -omega*Ezamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBz - 4*pi*Jz = %g\n", (speed_of_light_normalized*kw*Byamplitude*fieldScale - 4*pi*concentration*electron_charge_normalized*(VzamplitudeProton - VzamplitudeElectron))/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");

	double derivativJy = -electron_charge_normalized*concentration*(VyamplitudeProton - VyamplitudeElectron)*omega;
	fprintf(informationFile, "w*Jy amplitude = %g\n", derivativJy/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));

	double derivativeVelocitiesY = electron_charge_normalized*((Eyamplitude*fieldScale *((1.0/massProton) + (1.0/massElectron))) + B0.norm()*fieldScale*((VzamplitudeProton/massProton) + (VzamplitudeElectron/massElectron))/speed_of_light_normalized);
	fprintf(informationFile, "dJy/dt amplitude = %g\n", electron_charge_normalized*concentration*derivativeVelocitiesY/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	double derivativJz = electron_charge_normalized*concentration*(VzamplitudeProton - VzamplitudeElectron)*omega;
	fprintf(informationFile, "w*Jz amplitude = %g\n", derivativJz/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));

	double derivativeVelocitiesZ = electron_charge_normalized*((Ezamplitude*fieldScale *((1.0/massProton) + (1.0/massElectron))) - B0.norm()*fieldScale*((VyamplitudeProton/massProton) + (VyamplitudeElectron/massElectron))/speed_of_light_normalized);
	fprintf(informationFile, "dJz/dt amplitude = %g\n", electron_charge_normalized*concentration*derivativeVelocitiesZ/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");

	double derivativVyp = -omega*VyamplitudeProton;
	fprintf(informationFile, "-w*Vyp amplitude = %g\n", derivativVyp*gyroradius/sqr(plasma_period));

	double derivativeVelocityProtonY = electron_charge_normalized*(Eyamplitude*fieldScale + B0.norm()*fieldScale*VzamplitudeProton/speed_of_light_normalized)/massProton;
	fprintf(informationFile, "dVyp/dt amplitude = %g\n", derivativeVelocityProtonY*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVzp = omega*VzamplitudeProton;
	fprintf(informationFile, "w*Vzp amplitude = %g\n", derivativVzp*gyroradius/sqr(plasma_period));

	double derivativeVelocityProtonZ = electron_charge_normalized*(Ezamplitude*fieldScale - B0.norm()*fieldScale*VyamplitudeProton/speed_of_light_normalized)/massProton;
	fprintf(informationFile, "dVzp/dt amplitude = %g\n", derivativeVelocityProtonZ*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVye = -omega*VyamplitudeElectron;
	fprintf(informationFile, "-w*Vye amplitude = %g\n", derivativVye*gyroradius/sqr(plasma_period));

	double derivativeVelocityElectronY = -electron_charge_normalized*(Eyamplitude*fieldScale + B0.norm()*fieldScale*VzamplitudeElectron/speed_of_light_normalized)/massElectron;
	fprintf(informationFile, "dVye/dt amplitude = %g\n", derivativeVelocityElectronY*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVze = omega*VzamplitudeElectron;
	fprintf(informationFile, "w*Vze amplitude = %g\n", derivativVze*gyroradius/sqr(plasma_period));

	double derivativeVelocityElectronZ = -electron_charge_normalized*(Ezamplitude*fieldScale - B0.norm()*fieldScale*VyamplitudeElectron/speed_of_light_normalized)/massElectron;
	fprintf(informationFile, "dVze/dt amplitude = %g\n", derivativeVelocityElectronZ*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	fclose(informationFile);
}

void Simulation::initializeRotatedAlfvenWave(int wavesCount, double amplitudeRelation) {
	printf("initialization alfven wave\n");
	E0 = Vector3d(0, 0, 0);

	informationFile = fopen("./output/information.dat", "a");

	double alfvenV = B0.norm()*fieldScale / sqrt(4 * pi * density);
	if (alfvenV > speed_of_light_normalized) {
		printf("alfven velocity > c\n");
		fprintf(informationFile, "alfven velocity > c\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "alfvenV/c = %15.10g > 1\n", alfvenV/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	fprintf(informationFile, "alfven V = %lf\n", alfvenV*gyroradius/plasma_period);
	fprintf(informationFile, "alfven V/c = %lf\n", alfvenV/speed_of_light_normalized);
	printf("alfven V = %lf\n", alfvenV*gyroradius/plasma_period);
	printf("alfven V/c = %lf\n", alfvenV/speed_of_light_normalized);

	double kx = wavesCount * 2 * pi / xsize;
	double ky = wavesCount * 2 * pi / ysize;
	double kz = wavesCount * 2 * pi / zsize;

	double kw = sqrt(kx*kx + ky*ky + kz*kz);

	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;

	omegaPlasmaProton = sqrt(4*pi*concentration*electron_charge_normalized*electron_charge_normalized/massProton);
	omegaPlasmaElectron = sqrt(4*pi*concentration*electron_charge_normalized*electron_charge_normalized/massElectron);
	omegaGyroProton = B0.norm()*fieldScale*electron_charge_normalized/(massProton*speed_of_light_normalized);
	omegaGyroElectron = B0.norm()*fieldScale*electron_charge_normalized/(massElectron*speed_of_light_normalized);

	if(omegaGyroProton < 5*speed_of_light_normalized*kw){
		printf("omegaGyroProton < 5*k*c\n");
		fprintf(informationFile, "omegaGyroProton < 5*k*c\n");
		//fclose(informationFile);
		//exit(0);
	}
	printf("omegaGyroProton/kc = %g\n", omegaGyroProton/(kw*speed_of_light_normalized));
	fprintf(informationFile, "omegaGyroProton/kc = %g\n", omegaGyroProton/(kw*speed_of_light_normalized));

	if(omegaPlasmaProton < 5*omegaGyroProton){
		printf("omegaPlasmaProton < 5*omegaGyroProton\n");
		fprintf(informationFile, "omegaPlasmaProton < 5*omegaGyroProton\n");
		//fclose(informationFile);
		//exit(0);
	}
	printf("omegaPlasmaProton/omegaGyroProton = %g\n", omegaPlasmaProton/omegaGyroProton);
	fprintf(informationFile, "omegaPlasmaProton/omegaGyroProton = %g\n", omegaPlasmaProton/omegaGyroProton);

	//w = q*kw*B/mP * 0.5*(sqrt(d)+-b)/a
	double b = speed_of_light_normalized*kw*(massProton - massElectron)/massProton;
	double discriminant = speed_of_light_normalized_sqr*kw*kw*sqr(massProton + massElectron) + 16*pi*concentration*sqr(electron_charge_normalized)*(massProton + massElectron)/sqr(massProton);
	double a = (kw*kw*speed_of_light_normalized_sqr*massProton*massElectron + 4*pi*concentration*sqr(electron_charge_normalized)*(massProton + massElectron))/sqr(massProton);

	if(discriminant < 0){
		printf("discriminant < 0\n");
		fprintf(informationFile, "discriminant < 0\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "discriminant = %15.10g\n", discriminant);
		fclose(errorLogFile);
		exit(0);
	}

	double fakeOmega = (kw*electron_charge_normalized*B0.norm()*fieldScale/massProton)*(sqrt(discriminant) - b)/(2.0*a);

	//a4*x^4 + a3*x^3 + a2*x^2 + a1*x + a0 = 0

	double a4 = sqr(speed_of_light_normalized_sqr*massProton*massElectron);
	a4 = a4*sqr(sqr(sqr(fakeOmega)));
	double a3 = -2*cube(speed_of_light_normalized_sqr)*sqr(kw*massElectron*massProton)
		- 8*pi*concentration*sqr(speed_of_light_normalized_sqr*electron_charge_normalized)*massElectron*massProton*(massElectron + massProton)
		- sqr(B0.norm()*fieldScale*speed_of_light_normalized*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron));
	a3 = a3*cube(sqr(fakeOmega));
	double a2 = sqr(sqr(speed_of_light_normalized_sqr*kw)*massProton*massElectron)
		+ 8*pi*cube(speed_of_light_normalized_sqr)*concentration*sqr(kw*electron_charge_normalized)*massProton*massElectron*(massProton + massElectron)
		+ 16*sqr(pi*speed_of_light_normalized_sqr*concentration*sqr(electron_charge_normalized)*(massProton + massElectron))
		+ 2*sqr(B0.norm()*fieldScale*speed_of_light_normalized_sqr*kw*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron))
		+ 8*pi*concentration*sqr(B0.norm()*fieldScale*speed_of_light_normalized*sqr(electron_charge_normalized))*(massProton + massElectron)
		+ sqr(sqr(B0.norm()*fieldScale*electron_charge_normalized));
	a2 = a2*sqr(sqr(fakeOmega));
	double a1 = - sqr(B0.norm()*fieldScale*cube(speed_of_light_normalized)*kw*kw*electron_charge_normalized)*(sqr(massProton) + sqr(massElectron))
		- 8*pi*concentration*sqr(B0.norm()*fieldScale*speed_of_light_normalized_sqr*kw*sqr(electron_charge_normalized))*(massProton + massElectron)
		- 2*sqr(speed_of_light_normalized*kw*sqr(B0.norm()*fieldScale*electron_charge_normalized));
	a1 = a1*sqr(fakeOmega);
	double a0 = sqr(sqr(B0.norm()*fieldScale*speed_of_light_normalized*kw*electron_charge_normalized));

	a4 = a4/a0;
	a3 = a3/a0;
	a2 = a2/a0;
	a1 = a1/a0;
	a0 = 1.0;

	printf("a4 = %g\n", a4);
	fprintf(informationFile, "a4 = %g\n", a4);
	printf("a3 = %g\n", a3);
	fprintf(informationFile, "a3 = %g\n", a3);
	printf("a2 = %g\n", a2);
	fprintf(informationFile, "a2 = %g\n", a2);
	printf("a1 = %g\n", a1);
	fprintf(informationFile, "a1 = %g\n", a1);
	printf("a0 = %g\n", a0);
	fprintf(informationFile, "a0 = %g\n", a0);

	double fakeOmega1 = kw*alfvenV;
	printf("fakeOmega = %g\n", fakeOmega1/plasma_period);
	fprintf(informationFile, "fakeOmega = %g\n", fakeOmega1/plasma_period);
	double realOmega2 = solve4orderEquation(a4, a3, a2, a1, a0, 1.0);
	if(realOmega2 < 0){
		printf("omega^2 < 0\n");
		fprintf(informationFile, "omega^2 < 0\n");
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "omega^2 = %15.10g > 1\n", realOmega2);
		fclose(errorLogFile);
		exit(0);
	}

	double error = (((a4*realOmega2 + a3)*realOmega2 + a2)*realOmega2 + a1)*realOmega2 + a0;
	printf("error = %15.10g\n", error);
	fprintf(informationFile, "error = %15.10g\n", error);
	//double 
	omega = sqrt(realOmega2)*fakeOmega;
	if(omega < 0){
		omega = - omega;
	}

	if(omega > speed_of_light_normalized*kw/5.0){
		printf("omega > k*c/5\n");
		fprintf(informationFile, "omega > k*c/5\n");
		printf("omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
		fprintf(informationFile, "omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
		//fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "omega/kc = %15.10g > 0.2\n", omega/(kw*speed_of_light_normalized));
		fclose(errorLogFile);
		//exit(0);
	}
	printf("omega = %g\n", omega/plasma_period);
	fprintf(informationFile, "omega = %g\n", omega/plasma_period);

	printf("omega/kc = %g\n", omega/(kw*speed_of_light_normalized));
	fprintf(informationFile, "omega/kc = %g\n", omega/(kw*speed_of_light_normalized));

	if(fabs(omega) > omegaGyroProton/2){
		printf("omega > omegaGyroProton/2\n");
		fprintf(informationFile, "omega > omegaGyroProton/2\n");
	}
	printf("omega/omegaGyroProton = %g\n", omega/omegaGyroProton);
	fprintf(informationFile, "omega/omegaGyroProton = %g\n", omega/omegaGyroProton);
	fclose(informationFile);

	checkFrequency(omega);

	informationFile = fopen("./output/information.dat", "a");
	//checkCollisionTime(omega);
	//checkMagneticReynolds(alfvenV);
	//checkDissipation(kw, alfvenV);

	double epsilonAmplitude = amplitudeRelation;

	double alfvenVReal = omega/kw;

	fprintf(informationFile, "alfven V real = %15.10g\n", alfvenVReal*gyroradius/plasma_period);
	fprintf(informationFile, "alfven V real x = %15.10g\n", alfvenVReal*(kx/kw)*gyroradius/plasma_period);

	//double 
	Bzamplitude = B0.norm() * epsilonAmplitude;

	double Omegae = omegaGyroElectron;
	double Omegae2 = Omegae*Omegae;
	double Omegap = omegaGyroProton;
	double Omegap2 = Omegap*Omegap;
	double omegae = omegaPlasmaElectron;
	double omegae2 = omegae*omegae;
	double omegap = omegaPlasmaProton;
	double omegap2 = omegap*omegap;

	double kc = kw*speed_of_light_normalized;
	double kc2 = kc*kc;

	double denominator = omega*omega - Omegae2 - (omegae2*omega*omega/(omega*omega - kc2));

	//double 
	VzamplitudeProton = -((1.0/(4*pi*concentration*electron_charge_normalized))*(kc + ((omegae2 + omegap2 - omega*omega)/kc) + (omegae2*Omegae2/(kc*denominator)))/((Omegae*omegae2*omega/((kc2 - omega*omega)*denominator)) + (Omegap/omega)))*Bzamplitude*fieldScale;
	//double 
	VzamplitudeElectron = (((electron_charge_normalized*omega*Omegae)/(massElectron*kc))*Bzamplitude*fieldScale + (omegae2*omega*omega/(kc2 - omega*omega))*VzamplitudeProton)/denominator;

	//double 
	Byamplitude = (4*pi*concentration*electron_charge_normalized/((omega*omega/kc) - kc))*(VzamplitudeElectron - VzamplitudeProton)/fieldScale;

	//double 
	VyamplitudeProton = -(Omegap/omega)*VzamplitudeProton - (electron_charge_normalized/(massProton*kc))*Bzamplitude*fieldScale;
	//double 
	VyamplitudeElectron = (Omegae/omega)*VzamplitudeElectron + (electron_charge_normalized/(massElectron*kc))*Bzamplitude*fieldScale;

	//double 
	Eyamplitude = (omega/kc)*Bzamplitude;
	//double 
	Ezamplitude = -(omega/kc)*Byamplitude;

	double xshift = 0.0;

	//Eyamplitude = 0.0;
	//VzamplitudeElectron = 0.0;
	//VzamplitudeProton = 0.0;
	//Byamplitude = 0.0;

	double kxy = sqrt(kx*kx + ky*ky);
	double rotatedZortNorm = sqrt(kx*kx + ky*ky + sqr(kx*kx + ky*ky)/(kz*kz));
	double matrixzz = (kx*kx + ky*ky)/(kz*rotatedZortNorm);

	Matrix3d rotationMatrix = Matrix3d(kx/kw, -ky/kxy, -kx/rotatedZortNorm,
									   ky/kw, kx/kxy, -ky/rotatedZortNorm,
									   kz/kw, 0, matrixzz);

	if(kz == 0){
		rotationMatrix = Matrix3d(kx/kw, -ky/kw, 0,
								  ky/kw, kx/kw, 0,
								  0, 0, 1);
	}

	for (int i = 0; i < xnumber + 1; ++i) {
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k].x = 0;
				Efield[i][j][k].y = Eyamplitude * cos(kx * xgrid[i] + ky*ygrid[j] + kz*zgrid[k]);
				Efield[i][j][k].z = Ezamplitude * sin(kx * xgrid[i] + ky*ygrid[j] + kz*zgrid[k]);
				Efield[i][j][k] = rotationMatrix*Efield[i][j][k];
				explicitEfield[i][j][k] = Efield[i][j][k];
				tempEfield[i][j][k] = Efield[i][j][k];
				newEfield[i][j][k] = Efield[i][j][k];
			}
		}
	}

	for(int k = 0; k < znumber; ++k){
		for(int j = 0; j < ynumber; ++j){
			Efield[xnumber][j][k] = Efield[0][j][k];
			tempEfield[xnumber][j][k] = Efield[0][j][k];
			newEfield[xnumber][j][k] = Efield[0][j][k];
			explicitEfield[xnumber][j][k] = explicitEfield[0][j][k];
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber; ++j){
			Efield[i][j][znumber] = Efield[i][j][0];
			tempEfield[i][j][znumber] = Efield[i][j][0];
			newEfield[i][j][znumber] = Efield[i][j][0];
			explicitEfield[i][j][znumber] = explicitEfield[i][j][0];
		}
	}

	for(int k = 0; k < znumber + 1; ++k){
		for(int i = 0; i < xnumber + 1; ++i){
			Efield[i][ynumber][k] = Efield[i][0][k];
			tempEfield[i][ynumber][k] = Efield[i][0][k];
			newEfield[i][ynumber][k] = Efield[i][0][k];
			explicitEfield[i][ynumber][k] = explicitEfield[i][0][k];
		}
	}

	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k].x = B0.norm();
				Bfield[i][j][k].y = Byamplitude * sin(kx * middleXgrid[i] + ky * middleYgrid[j] + kz * middleZgrid[k]);
				Bfield[i][j][k].z = Bzamplitude * cos(kx * middleXgrid[i] + ky * middleYgrid[j] + kz * middleZgrid[k]);
				Bfield[i][j][k] = rotationMatrix * Bfield[i][j][k];
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}

	if(fabs(VzamplitudeProton) > speed_of_light_normalized){
		printf("VzamplitudeProton > speed_of_light_normalized\n");
		fprintf(informationFile, "VzamplitudeProton > speed_of_light_normalized\n");
		printf("VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
		fprintf(informationFile, "VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VzamplitudeProton/c = %15.10g > 1\n", VzamplitudeProton/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	printf("VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);
	fprintf(informationFile, "VzamplitudeProton/c = %g\n", VzamplitudeProton/speed_of_light_normalized);

	if(fabs(VzamplitudeElectron) > speed_of_light_normalized){
		printf("VzamplitudeElectron > speed_of_light_normalized\n");
		fprintf(informationFile, "VzamplitudeElectron > speed_of_light_normalized\n");
		printf("VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
		fprintf(informationFile, "VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VzamplitudeElectron/c = %15.10g > 1\n", VzamplitudeElectron/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	printf("VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);
	fprintf(informationFile, "VzamplitudeElectron/c = %g\n", VzamplitudeElectron/speed_of_light_normalized);

	if(fabs(VyamplitudeProton) > speed_of_light_normalized){
		printf("VyamplitudeProton > speed_of_light_normalized\n");
		fprintf(informationFile, "VyamplitudeProton > speed_of_light_normalized\n");
		printf("VyamplitudeProton/c = %g\n", VyamplitudeProton/speed_of_light_normalized);
		fprintf(informationFile, "VyamplitudeProton/c = %g\n", VyamplitudeProton/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VyamplitudeProton/c = %15.10g > 1\n", VyamplitudeProton/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}

	if(fabs(VyamplitudeElectron) > speed_of_light_normalized){
		printf("VyamplitudeElectron > speed_of_light_normalized\n");
		fprintf(informationFile, "VyamplitudeElectron > speed_of_light_normalized\n");
		printf("VyamplitudeElectron/c = %g\n", VyamplitudeElectron/speed_of_light_normalized);
		fprintf(informationFile, "VyamplitudeElectron/c = %g\n", VyamplitudeElectron/speed_of_light_normalized);
		fclose(informationFile);
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "VyamplitudeElectron/c = %15.10g > 1\n", VyamplitudeElectron/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}

	//k > 0, w > 0, kx-wt, Bz > 0, Vz < 0, Vyp > 0, Vye < 0

	for (int pcount = 0; pcount < particles.size(); ++pcount) {
		Particle* particle = particles[pcount];
		Vector3d velocity = particle->velocity(speed_of_light_normalized);
		int xn = (particle->coordinates.x - xgrid[0])/deltaX;
		double rightWeight = (particle->coordinates.x - xgrid[xn])/deltaX;
		double leftWeight = (xgrid[xn+1] - particle->coordinates.x)/deltaX;
		if (particle->type == PROTON) {
			velocity = (Vector3d(0, 0, 1) * (VzamplitudeProton) * cos(kx * particle->coordinates.x + ky * particle->coordinates.y +kz * particle->coordinates.z) + Vector3d(0, 1, 0) * VyamplitudeProton * sin(kx * particle->coordinates.x + ky * particle->coordinates.y +kz * particle->coordinates.z));
			//velocity = Vector3d(0, 0, 1) * (VzamplitudeProton) * (leftWeight*(cos(kw * (xgrid[xn] - xshift)) + rightWeight*cos(kw*(xgrid[xn+1] - xshift)))) + Vector3d(0, 1, 0) * VyamplitudeProton * (leftWeight*(sin(kw * (xgrid[xn] - xshift)) + rightWeight*sin(kw*(xgrid[xn+1] - xshift))));
		}
		if (particle->type == ELECTRON) {
			velocity = (Vector3d(0, 0, 1) * (VzamplitudeElectron) * cos(kx * particle->coordinates.x + ky * particle->coordinates.y +kz * particle->coordinates.z) + Vector3d(0, 1, 0) * VyamplitudeElectron * sin(kx * particle->coordinates.x + ky * particle->coordinates.y +kz * particle->coordinates.z));
			//velocity = Vector3d(0, 0, 1) * (VzamplitudeElectron) * (leftWeight*(cos(kw * (xgrid[xn] - xshift)) + rightWeight*cos(kw*(xgrid[xn+1] - xshift)))) + Vector3d(0, 1, 0) * VyamplitudeElectron * (leftWeight*(sin(kw * (xgrid[xn] - xshift)) + rightWeight*sin(kw*(xgrid[xn+1] - xshift))));
		}
		velocity = rotationMatrix * velocity;
		double beta = velocity.norm() / speed_of_light_normalized;
		particle->addVelocity(velocity, speed_of_light_normalized);
	}

	updateDeltaT();

	printf("dt/Talfven = %g\n", deltaT*omega/(2*pi));
	printf("dt = %g\n", deltaT*plasma_period);
	fprintf(informationFile, "dt/Talfven = %g\n", deltaT*omega/(2*pi));
	fprintf(informationFile, "dt = %g\n", deltaT*plasma_period);

	double Vthermal = sqrt(2*kBoltzman_normalized*temperature/massElectron);
	double thermalFlux = Vthermal*concentration*electron_charge_normalized/sqrt(1.0*particlesPerBin);
	double alfvenFlux = (VyamplitudeProton - VyamplitudeElectron)*concentration*electron_charge_normalized;
	if(thermalFlux > alfvenFlux/2){
		printf("thermalFlux > alfvenFlux/2\n");
		fprintf(informationFile, "thermalFlux > alfvenFlux/2\n");
	}
	printf("alfvenFlux/thermalFlux = %g\n", alfvenFlux/thermalFlux);
	fprintf(informationFile, "alfvenFlux/thermalFlux = %g\n", alfvenFlux/thermalFlux);
	double minDeltaT = deltaX/Vthermal;
	if(minDeltaT > deltaT){
		printf("deltaT < dx/Vthermal\n");
		fprintf(informationFile,  "deltaT < dx/Vthermal\n");

		//printf("deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
		//fprintf(informationFile, "deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
		
		//fclose(informationFile);
		//exit(0);
	}
	printf("deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
	fprintf(informationFile, "deltaT/minDeltaT =  %g\n", deltaT/minDeltaT);
	fprintf(informationFile, "\n");

	fprintf(informationFile, "Bz amplitude = %g\n", Bzamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "By amplitude = %g\n", Byamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "Vz amplitude p = %g\n", VzamplitudeProton*gyroradius/plasma_period);
	fprintf(informationFile, "Vz amplitude e = %g\n", VzamplitudeElectron*gyroradius/plasma_period);
	fprintf(informationFile, "Vy amplitude p = %g\n", VyamplitudeProton*gyroradius/plasma_period);
	fprintf(informationFile, "Vy amplitude e = %g\n", VyamplitudeElectron*gyroradius/plasma_period);
	fprintf(informationFile, "Ey amplitude = %g\n", Eyamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "Ez amplitude = %g\n", Ezamplitude*fieldScale/(plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "By/Ez = %g\n", Byamplitude/Ezamplitude);
	fprintf(informationFile, "Bz/Ey = %g\n", Bzamplitude/Eyamplitude);
	fprintf(informationFile, "4*pi*Jy amplitude = %g\n", 4*pi*concentration*electron_charge_normalized*(VyamplitudeProton - VyamplitudeElectron)/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBy amplitude = %g\n", speed_of_light_normalized*kw*Bzamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "4*pi*Jz amplitude = %g\n", 4*pi*concentration*electron_charge_normalized*(VzamplitudeProton - VzamplitudeElectron)/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBz amplitude = %g\n", speed_of_light_normalized*kw*Byamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative By amplitude = %g\n", -omega*Byamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "-c*rotEy = %g\n", speed_of_light_normalized*kw*Ezamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Bz amplitude = %g\n", omega*Bzamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "-c*rotEz = %g\n", speed_of_light_normalized*kw*Eyamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Ey amplitude = %g\n", omega*Eyamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBy - 4*pi*Jy = %g\n", (speed_of_light_normalized*kw*Bzamplitude*fieldScale - 4*pi*concentration*electron_charge_normalized*(VyamplitudeProton - VyamplitudeElectron))/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	fprintf(informationFile, "derivative Ez amplitude = %g\n", -omega*Ezamplitude*fieldScale/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "c*rotBz - 4*pi*Jz = %g\n", (speed_of_light_normalized*kw*Byamplitude*fieldScale - 4*pi*concentration*electron_charge_normalized*(VzamplitudeProton - VzamplitudeElectron))/(plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");

	double derivativJy = -electron_charge_normalized*concentration*(VyamplitudeProton - VyamplitudeElectron)*omega;
	fprintf(informationFile, "w*Jy amplitude = %g\n", derivativJy/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));

	double derivativeVelocitiesY = electron_charge_normalized*((Eyamplitude*fieldScale *((1.0/massProton) + (1.0/massElectron))) + B0.norm()*fieldScale*((VzamplitudeProton/massProton) + (VzamplitudeElectron/massElectron))/speed_of_light_normalized);
	fprintf(informationFile, "dJy/dt amplitude = %g\n", electron_charge_normalized*concentration*derivativeVelocitiesY/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");
	double derivativJz = electron_charge_normalized*concentration*(VzamplitudeProton - VzamplitudeElectron)*omega;
	fprintf(informationFile, "w*Jz amplitude = %g\n", derivativJz/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));

	double derivativeVelocitiesZ = electron_charge_normalized*((Ezamplitude*fieldScale *((1.0/massProton) + (1.0/massElectron))) - B0.norm()*fieldScale*((VyamplitudeProton/massProton) + (VyamplitudeElectron/massElectron))/speed_of_light_normalized);
	fprintf(informationFile, "dJz/dt amplitude = %g\n", electron_charge_normalized*concentration*derivativeVelocitiesZ/(plasma_period*plasma_period*plasma_period*sqrt(gyroradius)));
	fprintf(informationFile, "\n");

	double derivativVyp = -omega*VyamplitudeProton;
	fprintf(informationFile, "-w*Vyp amplitude = %g\n", derivativVyp*gyroradius/sqr(plasma_period));

	double derivativeVelocityProtonY = electron_charge_normalized*(Eyamplitude*fieldScale + B0.norm()*fieldScale*VzamplitudeProton/speed_of_light_normalized)/massProton;
	fprintf(informationFile, "dVyp/dt amplitude = %g\n", derivativeVelocityProtonY*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVzp = omega*VzamplitudeProton;
	fprintf(informationFile, "w*Vzp amplitude = %g\n", derivativVzp*gyroradius/sqr(plasma_period));

	double derivativeVelocityProtonZ = electron_charge_normalized*(Ezamplitude*fieldScale - B0.norm()*fieldScale*VyamplitudeProton/speed_of_light_normalized)/massProton;
	fprintf(informationFile, "dVzp/dt amplitude = %g\n", derivativeVelocityProtonZ*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVye = -omega*VyamplitudeElectron;
	fprintf(informationFile, "-w*Vye amplitude = %g\n", derivativVye*gyroradius/sqr(plasma_period));

	double derivativeVelocityElectronY = -electron_charge_normalized*(Eyamplitude*fieldScale + B0.norm()*fieldScale*VzamplitudeElectron/speed_of_light_normalized)/massElectron;
	fprintf(informationFile, "dVye/dt amplitude = %g\n", derivativeVelocityElectronY*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	double derivativVze = omega*VzamplitudeElectron;
	fprintf(informationFile, "w*Vze amplitude = %g\n", derivativVze*gyroradius/sqr(plasma_period));

	double derivativeVelocityElectronZ = -electron_charge_normalized*(Ezamplitude*fieldScale - B0.norm()*fieldScale*VyamplitudeElectron/speed_of_light_normalized)/massElectron;
	fprintf(informationFile, "dVze/dt amplitude = %g\n", derivativeVelocityElectronZ*gyroradius/sqr(plasma_period));
	fprintf(informationFile, "\n");

	fclose(informationFile);
}

void Simulation::initializeLangmuirWave(){
	double epsilon = 0.1;
	double kw = 2*2*pi/xsize;
	double omega = 2*pi;
	double langmuirV = omega/kw;

	checkDebyeParameter();
	informationFile = fopen("./output/information.dat", "a");
	fprintf(informationFile, "lengmuir V = %lf\n", langmuirV*gyroradius/plasma_period);
	fclose(informationFile);
	if(langmuirV > speed_of_light_normalized){
		printf("langmuirV > c\n");
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "langmuireV/c = %15.10g > 1\n", langmuirV/speed_of_light_normalized);
		fclose(errorLogFile);
		exit(0);
	}
	double concentration = density / (massProton + massElectron);
	printf("creating particles\n");
	int nproton = 0;
	int nelectron = 0;
	double weight = (concentration / particlesPerBin) * volumeB(0, 0, 0);
	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				double x;
				for (int l = 0; l < particlesPerBin; ++l) {
					ParticleTypes type;
					type = PROTON;
					Particle* particle = createParticle(particlesNumber, i, j, k, weight, type);
					nproton++;
					particles.push_back(particle);
					particlesNumber++;
					if (particlesNumber % 1000 == 0) {
						printf("create particle number %d\n", particlesNumber);
					}
				}
				for (int l = 0; l < particlesPerBin*(1 + epsilon*cos(kw*middleXgrid[i])); ++l) {
					ParticleTypes type;
					type = ELECTRON;
					Particle* particle = createParticle(particlesNumber, i, j, k, weight, type);
					nelectron++;
					particles.push_back(particle);
					particlesNumber++;
					if (particlesNumber % 1000 == 0) {
						printf("create particle number %d\n", particlesNumber);
					}
				}
			}
		}
	}	
	if(nproton != nelectron){
		printf("nproton != nelectron\n");
		int n;
		ParticleTypes type;
		if(nproton > nelectron){
			n = nproton - nelectron;
			type = ELECTRON;
		} else {
			n = nelectron - nproton;
			type = PROTON;
		}
		int i = 0;
		while(n > 0){
			Particle* particle = createParticle(particlesNumber, i, 0, 0, weight, type);
			particles.push_back(particle);
			particlesNumber++;
			++i;
			n--;
			if(i >= xnumber){
				i = 0;
			}
			if(type == PROTON){
				nproton++;
			} else {
				nelectron++;
			}
		}
	}
	if(nproton != nelectron){
		printf("nproton != nelectron\n");
		errorLogFile = fopen("./output/errorLog.dat", "w");
		fprintf(errorLogFile, "nproton = %d nelectron = %d\n", nproton, nelectron);
		fclose(errorLogFile);
		exit(0);
	}

	double chargeDensityAmplitude = epsilon*concentration*electron_charge_normalized;
	double Eamplitude = -4*pi*chargeDensityAmplitude/(kw*fieldScale);

	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k].x = Eamplitude*sin(kw*xgrid[i]);
				Efield[i][j][k].y = 0;
				Efield[i][j][k].z = 0;

				tempEfield[i] = Efield[i];
				explicitEfield[i] = Efield[i];
			}
		}
	}

	for(int j = 0; j < ynumber + 1; ++j){
		for(int k = 0; k < znumber + 1; ++k){
			Efield[xnumber][j][k] = Efield[0][j][k];
			tempEfield[xnumber][j][k] = tempEfield[0][j][k];
			explicitEfield[xnumber][j][k] = explicitEfield[0][j][k];
		}
	}

	double Vamplitude = electron_charge_normalized*Eamplitude*fieldScale/(massElectron*omega);

	for(int pcount = 0; pcount < particles.size(); ++pcount){
		Particle* particle = particles[pcount];
		if(particle->type == ELECTRON){
			Vector3d velocity = Vector3d(1, 0, 0)*Vamplitude*cos(kw*particle->coordinates.x);
			particle->addVelocity(velocity, speed_of_light_normalized);
		}
	}
}

void Simulation::initializeFluxFromRight(){
	//initializeAlfvenWave(10, 1.0E-4);
	for(int j = 0; j < ynumber + 1; ++j){
		for(int k = 0; k < znumber + 1; ++k){
			Efield[xnumber][j][k] = E0;
			tempEfield[xnumber][j][k] = E0;
			newEfield[xnumber][j][k] = E0;
			explicitEfield[xnumber][j][k] = E0;
			Efield[0][j][k] = E0;
			tempEfield[0][j][k] = E0;
			newEfield[0][j][k] = E0;
			explicitEfield[0][j][k] = E0;
		}
	}

	fieldsLorentzTransitionX(V0.x);

	for(int i = 0; i < particles.size(); ++i){
		Particle* particle = particles[i];
		particle->addVelocity(V0, speed_of_light_normalized);
	}

	double magneticEnergy = B0.scalarMult(B0)/(8*pi);
	double kineticEnergy = density*V0.scalarMult(V0)/2;

	informationFile = fopen("./output/information.dat","a");
	fprintf(informationFile, "magneticEnergy/kineticEnergy = %15.10g\n", magneticEnergy/kineticEnergy);
	printf("magneticEnergy/kinetikEnergy = %15.10g\n", magneticEnergy/kineticEnergy);
	fclose(informationFile);

}

void Simulation::fieldsLorentzTransitionX(const double& v){
	double gamma = 1.0/sqrt(1 - v*v/speed_of_light_normalized_sqr);
	for(int i = 0; i < xnumber; ++i){
		int prevI = i - 1;
		if(prevI < 0){
			if(boundaryConditionType == PERIODIC){
				prevI = xnumber - 1;
			} else {
				prevI = 0;
			}
		}
		for(int j = 0; j < ynumber; ++j){
			int prevJ = j - 1;
			if(prevJ < 0){
				prevJ = ynumber - 1;
			}
			for(int k = 0; k < znumber; ++k){
				int prevK = k - 1;
				if(prevK < 0){
					prevK = znumber - 1;
				}
				Vector3d middleB = (Bfield[i-1][prevJ][prevK] + Bfield[i-1][j][prevK] + Bfield[i-1][prevJ][k] + Bfield[i-1][j][k]
									+ Bfield[i][prevJ][prevK] + Bfield[i][j][prevK] + Bfield[i][prevJ][k] + Bfield[i][j][k])*0.125;
				newEfield[i][j][k].y = gamma*(Efield[i][j][k].y - v*middleB.z/speed_of_light_normalized);
				newEfield[i][j][k].z = gamma*(Efield[i][j][k].z + v*middleB.y/speed_of_light_normalized);
			}
		}
	}
	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Vector3d middleE = (Efield[i][j][k] + Efield[i][j+1][k] + Efield[i][j][k+1] + Efield[i][j+1][k+1]
									+ Efield[i+1][j][k] + Efield[i+1][j+1][k] + Efield[i+1][j][k+1] + Efield[i+1][j+1][k+1])*0.125;
				newBfield[i][j][k].y = gamma*(Bfield[i][j][k].y + v*middleE.z/speed_of_light_normalized);
				newBfield[i][j][k].z = gamma*(Bfield[i][j][k].z - v*middleE.y/speed_of_light_normalized);
			}
		}
	}

	for(int i = 0; i < xnumber; ++i){
		for(int k = 0; k < znumber; ++k){
			newEfield[i][ynumber][k] = newEfield[i][0][k];
		}
	}

	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber + 1; ++j){
			newEfield[i][j][znumber] = newEfield[i][j][0];
		}
	}

	if(boundaryConditionType == PERIODIC){
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				newEfield[xnumber][j][k] = newEfield[0][j][k];
			}
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k] = newEfield[i][j][k];
				tempEfield[i][j][k] = newEfield[i][j][k];
				explicitEfield[i][j][k] = newEfield[i][j][k];
			}
		}
	}
	
	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k] = newBfield[i][j][k];
			}
		}
	}
}

void Simulation::createArrays() {
	printf("creating arrays\n");
	xgrid = new double[xnumber + 1];
	ygrid = new double[ynumber + 1];
	zgrid = new double[znumber + 1];

	middleXgrid = new double[xnumber];
	middleYgrid = new double[ynumber];
	middleZgrid = new double[znumber];

	Efield = new Vector3d**[xnumber + 1];
	newEfield = new Vector3d**[xnumber + 1];
	tempEfield = new Vector3d**[xnumber + 1];
	explicitEfield = new Vector3d**[xnumber + 1];
	rotB = new Vector3d**[xnumber + 1];
	Ederivative = new Vector3d**[xnumber + 1];
	Bfield = new Vector3d**[xnumber];
	newBfield = new Vector3d**[xnumber];
	//tempBfield = new Vector3d**[xnumber];


	divergenceCleaningField = new double***[xnumber];
	divergenceCleaningPotential = new double***[xnumber];

	for (int i = 0; i < xnumber; ++i) {
		Bfield[i] = new Vector3d*[ynumber];
		newBfield[i] = new Vector3d*[ynumber];
		for(int j = 0; j < ynumber; ++j){
			Bfield[i][j] = new Vector3d[znumber];
			newBfield[i][j] = new Vector3d[znumber];
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][j] = Vector3d(0, 0, 0);
				newBfield[i][j][k] = Vector3d(0, 0, 0);
			}
		}
	}

	for (int i = 0; i < xnumber + 1; ++i) {
		Efield[i] = new Vector3d*[ynumber+1];
		newEfield[i] = new Vector3d*[ynumber+1];
		tempEfield[i] = new Vector3d*[ynumber+1];
		explicitEfield[i] = new Vector3d*[ynumber+1];
		rotB[i] = new Vector3d*[ynumber + 1];
		Ederivative[i] = new Vector3d*[ynumber + 1];
		for(int j = 0; j < ynumber + 1; ++j){
			Efield[i][j] = new Vector3d[znumber+1];
			newEfield[i][j] = new Vector3d[znumber+1];
			tempEfield[i][j] = new Vector3d[znumber+1];
			explicitEfield[i][j] = new Vector3d[znumber+1];
			rotB[i][j] = new Vector3d[znumber + 1];
			Ederivative[i][j] = new Vector3d[znumber + 1];
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k] = Vector3d(0, 0, 0);
				newEfield[i][j][k] = Vector3d(0, 0, 0);
				tempEfield[i][j][k] = Vector3d(0, 0, 0);
				explicitEfield[i][j][k] = Vector3d(0, 0, 0);
				rotB[i][j][k] = Vector3d(0, 0, 0);
				Ederivative[i][j][k] = Vector3d(0, 0, 0);
			}
		}
	}

	maxwellEquationMatrix = new std::vector<MatrixElement>***[xnumber];
	maxwellEquationRightPart = new double***[xnumber];
	for (int i = 0; i < xnumber; ++i) {
		maxwellEquationMatrix[i] = new std::vector<MatrixElement>**[ynumber];
		maxwellEquationRightPart[i] = new double**[ynumber];
		for(int j = 0; j < ynumber; ++j){
			maxwellEquationMatrix[i][j] = new std::vector<MatrixElement>*[znumber];
			maxwellEquationRightPart[i][j] = new double*[znumber];
			for(int k = 0; k < znumber; ++k){
				maxwellEquationMatrix[i][j][k] = new std::vector<MatrixElement>[maxwellEquationMatrixSize];
				maxwellEquationRightPart[i][j][k] = new double[maxwellEquationMatrixSize];
			}
		}
	}

	divergenceCleanUpMatrix = new std::vector<MatrixElement>***[xnumber];
	divergenceCleanUpRightPart = new double***[xnumber];

	divergenceCleaningField = new double***[xnumber];
	divergenceCleaningPotential = new double***[xnumber];

	for (int i = 0; i < xnumber; ++i) {
		divergenceCleanUpMatrix[i] = new std::vector<MatrixElement>**[ynumber];
		divergenceCleanUpRightPart[i] = new double**[ynumber];
		divergenceCleaningField[i] = new double**[ynumber];
		divergenceCleaningPotential[i] = new double**[ynumber];
		for(int j = 0; j < ynumber; ++j) {
			divergenceCleanUpMatrix[i][j] = new std::vector<MatrixElement>*[znumber];
			divergenceCleanUpRightPart[i][j] = new double*[znumber];
			divergenceCleaningField[i][j] = new double*[xnumber];
			divergenceCleaningPotential[i][j] = new double*[xnumber];
			for(int k = 0; k < znumber; ++k) {
				divergenceCleaningField[i][j][j] = new double[3];
				divergenceCleaningPotential[i][j][k] = new double[1];
				divergenceCleanUpMatrix[i][j][k] = new std::vector<MatrixElement>[3];
				divergenceCleanUpRightPart[i][j][k] = new double[3];
			}
		}
	}

	particlesInBbin = new std::vector<Particle*>**[xnumber];
	particlesInEbin = new std::vector<Particle*>**[xnumber + 1];

	for(int i = 0; i < xnumber; ++i){
		particlesInBbin[i] = new std::vector<Particle*>*[ynumber];
		for(int j = 0; j < ynumber; ++j){
			particlesInBbin[i][j] = new std::vector<Particle*>[znumber];
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		particlesInEbin[i] = new std::vector<Particle*>*[ynumber+1];
		for(int j = 0; j < ynumber + 1; ++j){
			particlesInEbin[i][j] = new std::vector<Particle*>[znumber+1];
		}
	}

	electronConcentration = new double**[xnumber];
	protonConcentration = new double**[xnumber];
	chargeDensity = new double**[xnumber];
	velocityBulk = new Vector3d**[xnumber];
	velocityBulkElectron = new Vector3d**[xnumber];
	electricDensity = new double**[xnumber];
	pressureTensor = new Matrix3d**[xnumber];

	for (int i = 0; i < xnumber; ++i) {
		electronConcentration[i] = new double*[ynumber];
		protonConcentration[i] = new double*[ynumber];
		chargeDensity[i] = new double*[ynumber];
		velocityBulk[i] = new Vector3d*[ynumber];
		velocityBulkElectron[i] = new Vector3d*[ynumber];
		electricDensity[i] = new double*[ynumber];
		pressureTensor[i] = new Matrix3d*[ynumber];
		for(int j = 0; j < ynumber; ++j){
			electronConcentration[i][j] = new double[znumber];
			protonConcentration[i][j] = new double[znumber];
			chargeDensity[i][j] = new double[znumber];
			velocityBulk[i][j] = new Vector3d[znumber];
			velocityBulkElectron[i][j] = new Vector3d[znumber];
			electricDensity[i][j] = new double[znumber];
			pressureTensor[i][j] = new Matrix3d[znumber];
			for(int k = 0; k < znumber; ++k){
				electronConcentration[i][j][k] = 0;
				protonConcentration[i][j][k] = 0;
				chargeDensity[i][j][k] = 0;
				velocityBulk[i][j][k] = Vector3d(0, 0, 0);
				velocityBulkElectron[i][j][k] = Vector3d(0, 0, 0);

				electricDensity[i][j][k] = 0;
				pressureTensor[i][j][k] = Matrix3d(0, 0, 0, 0, 0, 0, 0, 0, 0);
			}
		}
	}

	electricFlux = new Vector3d**[xnumber + 1];
	dielectricTensor = new Matrix3d**[xnumber + 1];
	externalElectricFlux = new Vector3d**[xnumber + 1];
	divPressureTensor = new Vector3d**[xnumber + 1];

	for (int i = 0; i < xnumber + 1; ++i) {
		electricFlux[i] = new Vector3d*[ynumber + 1];
		dielectricTensor[i] = new Matrix3d*[ynumber + 1];
		externalElectricFlux[i] = new Vector3d*[ynumber + 1];
		divPressureTensor[i] = new Vector3d*[ynumber + 1];
		for(int j = 0; j < ynumber + 1; ++j){
			electricFlux[i][j] = new Vector3d[znumber + 1];
			dielectricTensor[i][j] = new Matrix3d[znumber + 1];
			externalElectricFlux[i][j] = new Vector3d[znumber + 1];
			divPressureTensor[i][j] = new Vector3d[znumber + 1];
			for(int k = 0; k < znumber + 1; ++k){
				electricFlux[i][j][k] = Vector3d(0, 0, 0);
				divPressureTensor[i][j][k]  = Vector3d(0, 0, 0);
				dielectricTensor[i][j][k] = Matrix3d(0, 0, 0, 0, 0, 0, 0, 0, 0);
			}
		}
	}
}

void Simulation::initializeTwoStream(){
	createParticles();
	collectParticlesIntoBins();
	double u = speed_of_light_normalized/5;
	Vector3d electronsVelocityPlus = Vector3d(0, u, 0);
	Vector3d electronsVelocityMinus = Vector3d(0, -u, 0);
	B0 = Vector3d(0, 0, 0);

	double Bamplitude = 1E-12 * (plasma_period * sqrt(gyroradius));
	//Bamplitude = 0;
	double Eamplitude = 0;

	checkDebyeParameter();

	double kw = 2*pi/xsize;

	informationFile = fopen("./output/information.dat","a");
	
	if(xsize*omegaPlasmaElectron/speed_of_light_normalized < 5){
		printf("xsize*omegaPlasmaElectron/speed_of_light_normalized < 5\n");
		fprintf(informationFile, "xsize*omegaPlasmaElectron/speed_of_light_normalized < 5\n");
	}
	printf("xsize*omegaPlasmaElectron/speed_of_light_normalized = %g\n", xsize*omegaPlasmaElectron/speed_of_light_normalized);
	fprintf(informationFile, "xsize*omegaPlasmaElectron/speed_of_light_normalized = %g\n", xsize*omegaPlasmaElectron/speed_of_light_normalized);

	if(deltaX*omegaPlasmaElectron/speed_of_light_normalized > 0.2){
		printf("deltaX*omegaPlasmaElectron/speed_of_light_normalized > 0.2\n");
		fprintf(informationFile, "deltaX*omegaPlasmaElectron/speed_of_light_normalized > 0.2\n");
	}
	printf("deltaX*omegaPlasmaElectron/speed_of_light_normalized = %g\n", xsize*omegaPlasmaElectron/speed_of_light_normalized);
	fprintf(informationFile, "deltaX*omegaPlasmaElectron/speed_of_light_normalized = %g\n", xsize*omegaPlasmaElectron/speed_of_light_normalized);

	if(kw > omegaPlasmaElectron/u){
		printf("k > omegaPlasmaElectron/u\n");
		fprintf(informationFile, "k > omegaPlasmaElectron/u\n");
	}

	printf("k u/omegaPlasmaElectron = %g\n", kw*u/omegaPlasmaElectron);
	fprintf(informationFile, "k u/omegaPlasmaElectron = %g\n", kw*u/omegaPlasmaElectron);
	fclose(informationFile);

	for(int i = 0; i < xnumber; ++i){
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				Bfield[i][j][k] = Vector3d(0, 1, 0)*Bamplitude*cos(kw*middleXgrid[i]);
				newBfield[i][j][k] = Bfield[i][j][k];
			}
		}
	}

	for(int i = 0; i < xnumber + 1; ++i){
		for(int j = 0; j < ynumber + 1; ++j){
			for(int k = 0; k < znumber + 1; ++k){
				Efield[i][j][k] = Vector3d(0, 0, 1)*Eamplitude*cos(kw*xgrid[i]);
				tempEfield[i][j][k] = Efield[i][j][k];
				newEfield[i][j][k] = Efield[i][j][k];
				explicitEfield[i][j][k] = Efield[i][j][k];
			}
		}
	}
	int electronCount = 0;
	for(int pcount = 0; pcount < particles.size(); ++pcount){
		Particle* particle = particles[pcount];
		if(particle->type == ELECTRON){
			if(electronCount % 2 == 0){
				particle->addVelocity(electronsVelocityPlus, speed_of_light_normalized);
			} else {
				particle->addVelocity(electronsVelocityMinus, speed_of_light_normalized);
			}
			electronCount++;
		}
	}
}

void Simulation::initializeExternalFluxInstability(){
	double alfvenV = B0.norm()*fieldScale/sqrt(4*pi*density);
	double concentration = density/(massProton + massElectron);
	double phaseV = 2*alfvenV;
	double kw = 2*pi/xsize;
	double omega = kw*phaseV;

	extJ = 0.001*electron_charge_normalized*alfvenV*concentration;
	checkDebyeParameter();

	double Byamplitude = 4*pi*sqr(alfvenV)*extJ/(speed_of_light_normalized*kw*(sqr(phaseV) - sqr(alfvenV)))/(plasma_period*sqrt(gyroradius));
	double Uyamplitude = B0.norm()*fieldScale*phaseV*extJ/(kw*density*speed_of_light_normalized*(sqr(phaseV) - sqr(alfvenV)))*(gyroradius/plasma_period);
	double cyclothronOmegaElectron = electron_charge_normalized*B0.norm()*fieldScale/(massElectron*speed_of_light_normalized);
	double cyclothronOmegaProton = electron_charge_normalized*B0.norm()*fieldScale/(massProton*speed_of_light_normalized);


	checkGyroRadius();
	informationFile = fopen("./output/information.dat", "a");
	fprintf(informationFile, "alfven V = %g\n", alfvenV*gyroradius/plasma_period);
	fprintf(informationFile, "phase V = %g\n", phaseV*gyroradius/plasma_period);
	fprintf(informationFile, "alfven V/c = %g\n", alfvenV/speed_of_light_normalized);
	fprintf(informationFile, "phase V/c = %g\n", phaseV/speed_of_light_normalized);
	fprintf(informationFile, "external flux = %g\n", extJ*plasma_period*plasma_period*sqrt(gyroradius));
	fprintf(informationFile, "By max amplitude = %g\n", Byamplitude);
	fprintf(informationFile, "Uy max amplitude = %g\n", Uyamplitude);
	fprintf(informationFile, "omega/omega_plasma = %g\n", omega);
	if(omega > cyclothronOmegaProton) {
		printf("omega > cyclothron Omega Proton\n");
		fprintf(informationFile, "omega > cyclothron Omega Proton\n");
	} else if(omega > cyclothronOmegaProton/100.0) {
		printf("omega > cyclothrone Omega Proton/100\n");
		fprintf(informationFile, "omega > cyclothron Omega Proton/100\n");
	}
	printf("omega/cyclothronOmega = %g\n", omega/cyclothronOmegaProton);
	fprintf(informationFile, "omega/cyclothronOmega = %g\n", omega/cyclothronOmegaProton);

	fclose(informationFile);
}

void Simulation::createFiles() {
	printf("creating files\n");
	protonTraectoryFile = fopen("./output/traectory_proton.dat", "w");
	fclose(protonTraectoryFile);
	electronTraectoryFile = fopen("./output/traectory_electron.dat", "w");
	fclose(electronTraectoryFile);
	distributionFile = fopen("./output/distribution_protons.dat", "w");
	fclose(distributionFile);
	EfieldFile = fopen("./output/Efield.dat", "w");
	fclose(EfieldFile);
	BfieldFile = fopen("./output/Bfield.dat", "w");
	fclose(BfieldFile);
	velocityFile = fopen("./output/velocity.dat", "w");
	fclose(velocityFile);
	velocityElectronFile = fopen("./output/velocity_electron.dat", "w");
	fclose(velocityElectronFile);
	Xfile = fopen("./output/Xfile.dat", "w");
	fclose(Xfile);
	Yfile = fopen("./output/Yfile.dat", "w");
	fclose(Yfile);
	Zfile = fopen("./output/Zfile.dat", "w");
	fclose(Zfile);
	generalFile = fopen("./output/general.dat", "w");
	fclose(generalFile);
	densityFile = fopen("./output/concentrations.dat", "w");
	fclose(densityFile);
	divergenceErrorFile = fopen("./output/divergence_error.dat", "w");
	fclose(divergenceErrorFile);
	informationFile = fopen("./output/information.dat", "w");
	fclose(informationFile);
	fluxFile = fopen("./output/fluxFile.dat", "w");
	fclose(fluxFile);
	rotBFile = fopen("./output/rotBFile.dat", "w");
	fclose(rotBFile);
	EderivativeFile = fopen("./output/EderivativeFile.dat", "w");
	fclose(EderivativeFile);
	dielectricTensorFile = fopen("./output/dielectricTensorFile.dat", "w");
	fclose(dielectricTensorFile);
	errorLogFile = fopen("./output/errorLog.dat", "w");
	fclose(errorLogFile);
	particleProtonsFile = fopen("./output/protons.dat", "w");
	fclose(particleProtonsFile);
	particleElectronsFile = fopen("./output/electrons.dat", "w");
	fclose(particleElectronsFile);
}

void Simulation::checkFrequency(double omega) {
	informationFile = fopen("./output/information.dat", "a");
	double cyclothronOmegaElectron = electron_charge_normalized*B0.norm()*fieldScale/(massElectron*speed_of_light_normalized);
	double cyclothronOmegaProton = electron_charge_normalized*B0.norm()*fieldScale/(massProton*speed_of_light_normalized);
	if(omega > cyclothronOmegaProton) {
		printf("omega > cyclothron Omega Proton\n");
		fprintf(informationFile, "omega > cyclothron Omega Proton\n");
	} else if(omega > cyclothronOmegaProton/100.0) {
		printf("omega > cyclothrone Omega Proton/100\n");
		fprintf(informationFile, "omega > cyclothron Omega Proton/100\n");
	}
	printf("omega/cyclothronOmega = %g\n", omega/cyclothronOmegaProton);
	fprintf(informationFile, "omega/cyclothronOmega = %g\n", omega/cyclothronOmegaProton);

	if(omega > 1.0){
		printf("omega > omega plasma\n");
		fprintf(informationFile, "omega > omega plasma\n");
	} else if (omega > 0.01) {
		printf("omega > omega plasma/100\n");
		fprintf(informationFile, "omega > omega plasma/100\n");
	}	
	printf("omega/omega plasma = %g\n", omega);
	fprintf(informationFile, "omega/omega plasma = %g\n", omega);

	fclose(informationFile);
}

void Simulation::checkDebyeParameter() {
	informationFile = fopen("./output/information.dat", "a");
	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;
	double superParticleCharge = electron_charge_normalized * weight;
	double superParticleConcentration = concentration / weight;
	double superParticleTemperature = temperature * weight;

	double debyeLength = 1 / sqrt(4 * pi * electron_charge_normalized * electron_charge_normalized * concentration / (kBoltzman_normalized * temperature));
	double debyeNumber = 4 * pi * cube(debyeLength) * concentration / 3;

	if(debyeLength > deltaX){
		printf("debye length > deltaX\n");
		fprintf(informationFile, "debye length > deltaX\n");
	}
	printf("debye length/deltaX = %g\n", debyeLength/deltaX);
	fprintf(informationFile, "debye length/deltaX = %g\n", debyeLength/deltaX);

	if (debyeNumber < 1.0) {
		printf("debye number < 1\n");
		fprintf(informationFile, "debye number < 1\n");
	} else if (debyeNumber < 100.0) {
		printf("debye number < 100\n");
		fprintf(informationFile, "debye number < 100\n");
	}
	printf("debye number = %g\n", debyeNumber);
	fprintf(informationFile, "debye number = %g\n", debyeNumber);

	double superParticleDebyeLength = 1 / sqrt(4 * pi * superParticleCharge * superParticleCharge * superParticleConcentration / (kBoltzman_normalized * superParticleTemperature));
	double superParticleDebyeNumber = 4 * pi * cube(superParticleDebyeLength) * superParticleConcentration / 3;

	if(superParticleDebyeLength > deltaX){
		printf("super particle debye length > deltaX\n");
		fprintf(informationFile, "super particle debye length > deltaX\n");
	}
	printf("super particle debye length/deltaX = %g\n", superParticleDebyeLength/deltaX);
	fprintf(informationFile, "super particle debye length/deltaX = %g\n", superParticleDebyeLength/deltaX);

	if (superParticleDebyeNumber < 1.0) {
		printf("superparticle debye number < 1\n");
		fprintf(informationFile, "superparticle debye number < 1\n");
	} else if (superParticleDebyeNumber < 100.0) {
		printf("superparticle debye number < 100\n");
		fprintf(informationFile, "superparticle debye number < 100\n");
	}
	printf("superparticle debye number = %g\n", superParticleDebyeNumber);
	fprintf(informationFile, "superparticle debye number = %g\n", superParticleDebyeNumber);
	fclose(informationFile);
}

void Simulation::checkGyroRadius(){
	informationFile = fopen("./output/information.dat", "a");

	if(B0.norm() > 0){
		double thermalMomentumElectron = sqrt(massElectron*kBoltzman_normalized*temperature) + massElectron*V0.norm();
		double gyroRadiusElectron = thermalMomentumElectron*speed_of_light_normalized/(electron_charge_normalized * B0.norm());
		double thermalMomentumProton = sqrt(massProton*kBoltzman_normalized*temperature) + massProton*V0.norm();
		double gyroRadiusProton = thermalMomentumProton*speed_of_light_normalized/(electron_charge_normalized * B0.norm());
		if(deltaX > 0.5*gyroRadiusElectron){
			printf("deltaX > 0.5*gyroRadiusElectron\n");
			fprintf(informationFile, "deltaX > 0.5*gyroRadiusElectron\n");
		}

		printf("deltaX/gyroRadiusElectron = %g\n", deltaX/gyroRadiusElectron);
		fprintf(informationFile, "deltaX/gyroRadiusElectron = %g\n", deltaX/gyroRadiusElectron);

		if(xsize < 2*gyroRadiusProton){
			printf("xsize < 2*gyroRadiusProton\n");
			fprintf(informationFile, "xsize < 2*gyroRadiusProton\n");
		}

		printf("xsize/gyroRadiusProton= %g\n", xsize/gyroRadiusProton);
		fprintf(informationFile, "xsize/gyroRadiusProton = %g\n", xsize/gyroRadiusProton);
	}

	fclose(informationFile);
}

void Simulation::checkCollisionTime(double omega) {
	informationFile = fopen("./output/information.dat", "a");
	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;
	double superParticleCharge = electron_charge_normalized * weight;
	double superParticleConcentration = concentration / weight;
	double superParticleTemperature = temperature * weight;

	double qLog = 15;
	double nuElectronIon = 4 * sqrt(2 * pi) * qLog * power(electron_charge_normalized, 4) * (concentration) / (3 * sqrt(massElectron) * sqrt(cube(kBoltzman_normalized * temperature)));
	double collisionlessParameter = omega / nuElectronIon;

	if (collisionlessParameter < 1.0) {
		printf("collisionlessParameter < 1\n");
		fprintf(informationFile, "collisionlessParameter < 1\n");
	} else if (collisionlessParameter < 100.0) {
		printf("collisionlessParameter < 100\n");
		fprintf(informationFile, "collisionlessParameter < 100\n");
	}
	printf("collisionlessParameter = %g\n", collisionlessParameter);
	fprintf(informationFile, "collisionlessParameter = %g\n", collisionlessParameter);

	double superParticleNuElectronIon = 4 * sqrt(2 * pi) * qLog * power(electron_charge_normalized * weight, 4) * (superParticleConcentration) / (3 * sqrt(massElectron * weight) * sqrt(cube(kBoltzman_normalized * superParticleTemperature)));
	double superParticleCollisionlessParameter = omega / superParticleNuElectronIon;

	if (superParticleCollisionlessParameter < 1.0) {
		printf("superParticleCollisionlessParameter < 1\n");
		fprintf(informationFile, "superParticleCollisionlessParameter < 1\n");
	} else if (superParticleCollisionlessParameter < 100.0) {
		printf("superParticleCollisionlessParameter < 100\n");
		fprintf(informationFile, "superParticleCollisionlessParameter < 100\n");
	}
	printf("superParticleCollisionlessParameter = %g\n", superParticleCollisionlessParameter);
	fprintf(informationFile, "superParticleCollisionlessParameter = %g\n", superParticleCollisionlessParameter);
	fclose(informationFile);
}

void Simulation::checkMagneticReynolds(double v) {
	informationFile = fopen("./output/information.dat", "a");
	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;
	double superParticleCharge = electron_charge_normalized * weight;
	double superParticleConcentration = concentration / weight;
	double superParticleTemperature = temperature * weight;

	double qLog = 15;
	double conductivity = (3 * massProton * sqrt(massElectron) * sqrt(cube(kBoltzman_normalized * temperature))) / (sqr(massElectron) * 4 * sqrt(2 * pi) * qLog * sqr(electron_charge_normalized));

	double magneticReynolds = conductivity * v * 0.1 * xsize;

	if (magneticReynolds < 1.0) {
		printf("magneticReynolds < 1\n");
		fprintf(informationFile, "magneticReynolds < 1\n");
	} else if (magneticReynolds < 100.0) {
		printf("magneticReynolds < 100\n");
		fprintf(informationFile, "magneticReynolds < 100\n");
	}
	printf("magnetic Reynolds = %g\n", magneticReynolds);
	fprintf(informationFile, "magnetic Reynolds = %g\n", magneticReynolds);

	double superParticleConductivity = (3 * massProton * weight * sqrt(massElectron * weight) * sqrt(cube(kBoltzman_normalized * superParticleTemperature))) / (sqr(massElectron * weight) * 4 * sqrt(2 * pi) * qLog * sqr(electron_charge_normalized * weight));

	double superParticleMagneticReynolds = superParticleConductivity * v * 0.1 * xsize;

	if (superParticleMagneticReynolds < 1.0) {
		printf("superParticleMagneticReynolds < 1\n");
		fprintf(informationFile, "superParticleMagneticReynolds < 1\n");
	} else if (superParticleMagneticReynolds < 100.0) {
		printf("superParticleMagneticReynolds < 100\n");
		fprintf(informationFile, "superParticleMagneticReynolds < 100\n");
	}
	printf("superparticle magnetic Reynolds = %g\n", superParticleMagneticReynolds);
	fprintf(informationFile, "superparticle magnetic Reynolds = %g\n", superParticleMagneticReynolds);
	fclose(informationFile);
}

void Simulation::checkDissipation(double k, double alfvenV) {
	informationFile = fopen("./output/information.dat", "a");
	double omega = k * alfvenV;

	double concentration = density / (massProton + massElectron);
	double weight = concentration * volumeB(0, 0, 0) / particlesPerBin;
	double superParticleCharge = electron_charge_normalized * weight;
	double superParticleConcentration = concentration / weight;
	double superParticleTemperature = temperature * weight;

	double qLog = 15;
	double conductivity = (3 * massProton * sqrt(massElectron) * sqrt(cube(kBoltzman_normalized * temperature))) / (sqr(massElectron) * 4 * sqrt(2 * pi) * qLog * sqr(electron_charge_normalized));

	double nuMagnetic = speed_of_light_normalized_sqr / (4 * pi * conductivity);

	double kdissipation = omega * omega * nuMagnetic / (2 * cube(alfvenV));

	if (kdissipation > k) {
		printf("kdissipation > k\n");
		fprintf(informationFile, "kdissipation > k\n");
	} else if (kdissipation > 0.1 * k) {
		printf("kdissipation > 0.1*k\n");
		fprintf(informationFile, "kdissipation > 0.1*k\n");
	}
	printf("kdissipation/k = %g\n", kdissipation / k);
	fprintf(informationFile, "kdissipation/k = %g\n", kdissipation / k);

	double superParticleConductivity = (3 * massProton * weight * sqrt(massElectron * weight) * sqrt(cube(kBoltzman_normalized * superParticleTemperature))) / (sqr(massElectron * weight) * 4 * sqrt(2 * pi) * qLog * sqr(electron_charge_normalized * weight));

	double superParticleNuMagnetic = speed_of_light_normalized_sqr / (4 * pi * superParticleConductivity);

	double superParticleKdissipation = omega * omega * superParticleNuMagnetic / (2 * cube(alfvenV));

	if (superParticleKdissipation > k) {
		printf("super particle kdissipation > k\n");
		fprintf(informationFile, "super particle kdissipation > k\n");
	} else if (superParticleKdissipation > 0.1 * k) {
		printf("super particle kdissipation > 0.1*k\n");
		fprintf(informationFile, "super particle kdissipation > 0.1*k\n");
	}
	printf("super particle kdissipation/k = %g\n", superParticleKdissipation / k);
	fprintf(informationFile, "super particle kdissipation/k = %g\n", superParticleKdissipation / k);
	fclose(informationFile);
}

void Simulation::createParticles() {
	printf("creating particles\n");
	double concentration = density/(massProton + massElectron);
	int n = 0;
	for (int i = 0; i < xnumber; ++i) {
		for(int j = 0; j < ynumber; ++j){
			for(int k = 0; k < znumber; ++k){
				double weight = (concentration / particlesPerBin) * volumeB(i, j, k);
				double x = xgrid[i] + 0.0001*deltaX;
				double y = ygrid[j] + 0.0001*deltaY;
				double z = zgrid[k] + 0.0001*deltaZ;
				double deltaXParticles = deltaX/particlesPerBin;
				double deltaYParticles = deltaY/particlesPerBin;
				double deltaZParticles = deltaZ/particlesPerBin;
				for (int l = 0; l < 2 * particlesPerBin; ++l) {
					ParticleTypes type;
					if (l % 2 == 0) {
						type = PROTON;
					} else {
						type = ELECTRON;
					}
					Particle* particle = createParticle(n, i, j, k, weight, type);
					//particle->x = middleXgrid[i];
					n++;
					/*if (l % 2 == 0) {
						x = particle->x;
					} else {
						particle->x= x;
					}*/
					int m = l/2;
					particle->coordinates.x = x + deltaXParticles*m;
					particle->coordinates.y = y + deltaYParticles*m;
					particle->coordinates.z = z + deltaZParticles*m;
					particles.push_back(particle);
					particlesNumber++;
					if (particlesNumber % 1000 == 0) {
						printf("create particle number %d\n", particlesNumber);
					}
				}
			}
		}
	}
}

Particle* Simulation::getFirstProton() {
	for (int pcount = 0; pcount < particles.size(); ++pcount) {
		Particle* particle = particles[pcount];
		if (particle->type == PROTON) {
			return particle;
		}
	}
	return NULL;
}

Particle* Simulation::getFirstElectron() {
	for (int pcount = 0; pcount < particles.size(); ++pcount) {
		Particle* particle = particles[pcount];
		if (particle->type == ELECTRON) {
			return particle;
		}
	}
	return NULL;
}

Particle* Simulation::createParticle(int n, int i, int j, int k, double weight, ParticleTypes type) {
	double charge = 0;
	double mass = 0;

	switch (type) {
	case PROTON:
		mass = massProton;
		charge = electron_charge_normalized;
		break;
	case ELECTRON:
		mass = massElectron;
		charge = -electron_charge_normalized;
		break;
	}

	double x = xgrid[i] +  deltaX * uniformDistribution();
	double y = ygrid[j] +  deltaY * uniformDistribution();
	double z = zgrid[k] +  deltaZ * uniformDistribution();

	double dx = deltaX / 2;
	double dy = deltaY / 2;
	double dz = deltaZ / 2;

	double energy = mass * speed_of_light_normalized_sqr;
	double p;

	double thetaParamter = kBoltzman_normalized * temperature / (mass * speed_of_light_normalized_sqr);

	if (thetaParamter < 0.01) {
		energy = maxwellDistribution(temperature, kBoltzman_normalized);
		p = sqrt(2*mass*energy);
	} else {
		energy = maxwellJuttnerDistribution(temperature, mass, speed_of_light_normalized, kBoltzman_normalized);
		p = sqrt(energy * energy - sqr(mass * speed_of_light_normalized_sqr)) / speed_of_light_normalized;

	}

	double pz = p * (2 * uniformDistribution() - 1);
	double phi = 2 * pi * uniformDistribution();
	double pnormal = sqrt(p * p - pz * pz);
	double px = pnormal * cos(phi);
	px = 0;
	double py = pnormal * sin(phi);

	Particle* particle = new Particle(n, mass, charge, weight, type, x, y, z, px, py, pz, dx, dy, dz);

	return particle;
}