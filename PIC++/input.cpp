#include <mpi.h>
#include "stdio.h"

//#include <crtdbg.h>

//#include "memory_debug.h"
#include "input.h"
#include "particle.h"
#include "vector3d.h"
#include "simulation.h"

#include "constants.h"
#include "mpi_util.h"
#include "paths.h"

Simulation readInput(FILE* inputFile, MPI_Comm& comm) {
	std::string outputDir = outputDirectory;
	int inputType;
	char ch = ' ';

	fscanf(inputFile, "%d", &inputType);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int xnumber;
	fscanf(inputFile, "%d", &xnumber);

	printf("scan xnumber = %d\n", xnumber);
	if (xnumber < 0) {
		printf("xnumber must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "xnumber must be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	int ynumber;
	fscanf(inputFile, "%d", &ynumber);

	if (ynumber < 0) {
		printf("ynumber must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "ynumber must be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	int znumber;
	fscanf(inputFile, "%d", &znumber);
	if (znumber < 0) {
		printf("znumber must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "znumber must be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double dx;
	fscanf(inputFile, "%lf", &dx);
	if (dx < 0) {
		printf("xsize must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "xsize must be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double dgamma;
	fscanf(inputFile, "%lf", &dgamma);

	if (dgamma < 0) {
		printf("temperature must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "v/c > 1 in setMomentumByV\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double Vx;
	fscanf(inputFile, "%lf", &Vx);

	double Vy;
	fscanf(inputFile, "%lf", &Vy);

	double Vz;
	fscanf(inputFile, "%lf", &Vz);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double sigma;
	fscanf(inputFile, "%lf", &sigma);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double Btheta;
	fscanf(inputFile, "%lf", &Btheta);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double Bphi;
	fscanf(inputFile, "%lf", &Bphi);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double Ex, Ey, Ez;
	fscanf(inputFile, "%lf %lf %lf", &Ex, &Ey, &Ez);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double initialElectronConcentration;
	fscanf(inputFile, "%lf", &initialElectronConcentration);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int maxIterations;
	fscanf(inputFile, "%d", &maxIterations);

	if (maxIterations < 0) {
		printf("max iterations must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "maxIterations mast be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double maxTime;
	fscanf(inputFile, "%lf", &maxTime);

	if (maxTime < 0) {
		printf("max time must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "maxTime must be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double preferedTimeStep;
	fscanf(inputFile, "%lf", &preferedTimeStep);

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int verbocity;
	fscanf(inputFile, "%d", &verbocity);

	if (verbocity < 0) {
		printf("verbocity must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "verbocity mast be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int writeIterationParameter;
	ch = ' ';
	fscanf(inputFile, "%d", &writeIterationParameter);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int writeGeneralParameter;
	ch = ' ';
	fscanf(inputFile, "%d", &writeGeneralParameter);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int writeTrajectoryParameter;
	ch = ' ';
	fscanf(inputFile, "%d", &writeTrajectoryParameter);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int writeParticleStep;
	ch = ' ';
	fscanf(inputFile, "%d", &writeParticleStep);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int smoothingCount;
	ch = ' ';
	fscanf(inputFile, "%d", &smoothingCount);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double smoothingParameter;
	ch = ' ';
	fscanf(inputFile, "%lf", &smoothingParameter);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int multiplyOutput;
	ch = ' ';
	fscanf(inputFile, "%d", &multiplyOutput);

	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}
	bool multiplyFileOutput = (multiplyOutput == 1);

	double massElectronInput;
	fscanf(inputFile, "%lf", &massElectronInput);

	if (verbocity < 0) {
		printf("verbocity must be > 0\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "verbocity mast be > 0\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	int typesNumber;
	fscanf(inputFile, "%d", &typesNumber);

	if (typesNumber < 1) {
		printf("typesNumber > 1\n");
		FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
		fprintf(errorLogFile, "typesNumber > 1\n");
		fclose(errorLogFile);
		MPI_Finalize();
		exit(0);
	}

	ch = ' ';
	while (ch != '\n') {
		fscanf(inputFile, "%c", &ch);
	}

	double* relativeConcentrations = new double[typesNumber];
	int* particlesPerBin = new int[typesNumber];

	for (int i = 0; i < typesNumber; ++i) {
		int particlePerBin;
		double typeConcentration;
		fscanf(inputFile, "%lf %d", &typeConcentration, &particlePerBin);
		//concentrations[i] = typeConcentration*initialElectronConcentration;
		relativeConcentrations[i] = typeConcentration;
		particlesPerBin[i] = particlePerBin;

		if (particlesPerBin[i] < 0) {
			printf("particlesPerBin[i] must be > 0\n");
			FILE* errorLogFile = fopen((outputDir + "errorLog.dat").c_str(), "w");
			fprintf(errorLogFile, "particlesPerBin[i] must be > 0\n");
			fclose(errorLogFile);
			MPI_Finalize();
			exit(0);
		}

		ch = ' ';
		while (ch != '\n') {
			fscanf(inputFile, "%c", &ch);
		}
	}


	int nprocs;
	MPI_Comm_size(comm, &nprocs);


	printf("finish read input\n");
	return Simulation(xnumber, ynumber, znumber, dx, dgamma, Vx, Vy, Vz, sigma, Btheta, Bphi, Ex, Ey, Ez, initialElectronConcentration,
	                  maxIterations, maxTime, writeIterationParameter, writeGeneralParameter, writeTrajectoryParameter, writeParticleStep, smoothingCount, smoothingParameter, multiplyFileOutput, typesNumber, particlesPerBin, relativeConcentrations, inputType, nprocs, verbocity,
	                  preferedTimeStep, massElectronInput, comm);
}

Simulation readBackup(const char* generalFileName, const char* EfileName, const char* BfileName,
                      const char* particlesFileName, MPI_Comm& comm) {
	int size;
	int rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	Simulation simulation;
	if (rank == 0) {
		FILE* generalFile = fopen(generalFileName, "r");
		simulation = Simulation();
		int inputType = 0;
		fscanf(generalFile, "%d", &inputType);
		if (inputType == 0) {
			simulation.inputType = CGS;
		} else if (inputType == 1) {
			simulation.inputType = Theoretical;
		} else {
			printf("input type must be 1 or 0\n");
			FILE* errorLogFile = fopen((simulation.outputDir + "errorLog.dat").c_str(), "w");
			fprintf(errorLogFile, "input type must be 1 or 0\n");
			fclose(errorLogFile);
			MPI_Finalize();
			exit(0);
		}
		fscanf(generalFile, "%d", &simulation.xnumberGeneral);
		fscanf(generalFile, "%d", &simulation.ynumber);
		fscanf(generalFile, "%d", &simulation.znumber);
		fscanf(generalFile, "%d", &simulation.particlesNumber);
		fscanf(generalFile, "%d", &simulation.types[0].particlesPerBin);
		fscanf(generalFile, "%d", &simulation.types[1].particlesPerBin);
		fscanf(generalFile, "%d", &simulation.types[2].particlesPerBin);
		fscanf(generalFile, "%d", &simulation.types[3].particlesPerBin);
		fscanf(generalFile, "%d", &simulation.types[4].particlesPerBin);
		fscanf(generalFile, "%d", &simulation.types[5].particlesPerBin);

		fscanf(generalFile, "%lf", &simulation.types[0].concentration);
		fscanf(generalFile, "%lf", &simulation.types[1].concentration);
		fscanf(generalFile, "%lf", &simulation.types[2].concentration);
		fscanf(generalFile, "%lf", &simulation.types[3].concentration);
		fscanf(generalFile, "%lf", &simulation.types[4].concentration);
		fscanf(generalFile, "%lf", &simulation.types[5].concentration);


		fscanf(generalFile, "%lf", &simulation.temperature);
		fscanf(generalFile, "%lf", &simulation.plasma_period);
		fscanf(generalFile, "%lf", &simulation.scaleFactor);

		fscanf(generalFile, "%lf", &simulation.time);
		fscanf(generalFile, "%lf", &simulation.maxTime);

		fscanf(generalFile, "%d", &simulation.currentIteration);
		fscanf(generalFile, "%d", &simulation.maxIteration);

		fscanf(generalFile, "%lf", &simulation.xsizeGeneral);
		fscanf(generalFile, "%lf", &simulation.ysizeGeneral);
		fscanf(generalFile, "%lf", &simulation.zsizeGeneral);
		fscanf(generalFile, "%lf", &simulation.theta);
		fscanf(generalFile, "%lf", &simulation.eta);

		int debugMode = 0;

		fscanf(generalFile, "%d", &debugMode);

		simulation.debugMode = (debugMode == 1);

		//int preserveChargeLocal = 0;

		//fscanf(generalFile, "%d", &preserveChargeLocal);

		//simulation.preserveChargeLocal = (preserveChargeLocal == 1);

		//int preserveChargeGlobal = 0;

		//fscanf(generalFile, "%d", &preserveChargeGlobal);

		//simulation.preserveChargeGlobal = (preserveChargeGlobal == 1);

		int solverType = 0;

		fscanf(generalFile, "%d", &solverType);

		if (solverType == 1) {
			simulation.solverType = IMPLICIT;
		} else {
			simulation.solverType = EXPLICIT;
		}

		int boundaryConditionType = 0;

		fscanf(generalFile, "%d", &boundaryConditionType);

		if (boundaryConditionType == 1) {
			simulation.boundaryConditionTypeX = PERIODIC;
		} else {
			simulation.boundaryConditionTypeX = SUPER_CONDUCTOR_LEFT;
		}

		fscanf(generalFile, "%d", &simulation.maxwellEquationMatrixSize);

		fscanf(generalFile, "%lf", &simulation.extJ);

		fscanf(generalFile, "%lf", &simulation.V0.x);
		fscanf(generalFile, "%lf", &simulation.V0.y);
		fscanf(generalFile, "%lf", &simulation.V0.z);
		fscanf(generalFile, "%lf", &simulation.E0.x);
		fscanf(generalFile, "%lf", &simulation.E0.y);
		fscanf(generalFile, "%lf", &simulation.E0.z);
		fscanf(generalFile, "%lf", &simulation.B0.x);
		fscanf(generalFile, "%lf", &simulation.B0.y);
		fscanf(generalFile, "%lf", &simulation.B0.z);

		fclose(generalFile);

		simulation.setSpaceForProc();

		simulation.deltaX = simulation.xsize / (simulation.xnumberAdded);
		simulation.deltaY = simulation.ysize / (simulation.ynumberAdded);
		simulation.deltaZ = simulation.zsize / (simulation.znumberAdded);

		simulation.deltaX2 = simulation.deltaX * simulation.deltaX;
		simulation.deltaY2 = simulation.deltaY * simulation.deltaY;
		simulation.deltaZ2 = simulation.deltaZ * simulation.deltaZ;

		simulation.createArrays();

		simulation.rescaleConstants();

		for (int i = 0; i <= simulation.xnumberAdded; ++i) {
			simulation.xgrid[i] = i * simulation.deltaX;
		}

		for (int i = 0; i <= simulation.ynumberAdded; ++i) {
			simulation.ygrid[i] = i * simulation.deltaY;
		}

		for (int i = 0; i <= simulation.znumberAdded; ++i) {
			simulation.zgrid[i] = i * simulation.deltaZ;
		}

		for (int i = 0; i < simulation.xnumberAdded; ++i) {
			simulation.middleXgrid[i] = (simulation.xgrid[i] + simulation.xgrid[i + 1]) / 2;
		}

		for (int i = 0; i < simulation.ynumberAdded; ++i) {
			simulation.middleYgrid[i] = (simulation.ygrid[i] + simulation.ygrid[i + 1]) / 2;
		}

		for (int i = 0; i < simulation.znumberAdded; ++i) {
			simulation.middleZgrid[i] = (simulation.zgrid[i] + simulation.zgrid[i + 1]) / 2;
		}

		sendInput(simulation, size);
	} else {
		simulation = recieveInput(comm);
		simulation.createArrays();
		simulation.rank = rank;
	}
	readFields(EfileName, BfileName, simulation);
	simulation.resetNewTempFields();
	readParticles(particlesFileName, simulation);

	return simulation;
}

void readFields(const char* EfileName, const char* BfileName, Simulation& simulation) {
	int size;
	int rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	for (int procCount = 0; procCount < size; ++procCount) {
		MPI_Barrier(MPI_COMM_WORLD);
		if (procCount == rank) {
			FILE* Bfile = fopen(BfileName, "r");
			for (int i = 0; i < simulation.xnumberGeneral; ++i) {
				for (int j = 0; j < simulation.ynumberGeneral; ++j) {
					for (int k = 0; k < simulation.znumberGeneral; ++k) {
						if (i >= simulation.firstAbsoluteXindex + simulation.xnumberAdded) {
							break;
						}
						double x;
						double y;
						double z;
						fscanf(Bfile, "%lf %lf %lf", &x, &y, &z);
						if (i >= simulation.firstAbsoluteXindex && i < simulation.firstAbsoluteXindex + simulation.xnumber) {
							simulation.Bfield[i][j][k] = Vector3d(x, y, z);
						}
					}
				}
			}
			fclose(Bfile);
		}
	}

	for (int procCount = 0; procCount < size; ++procCount) {
		MPI_Barrier(MPI_COMM_WORLD);
		if (procCount == rank) {
			FILE* Efile = fopen(EfileName, "r");
			for (int i = 0; i < simulation.xnumberGeneral + 1; ++i) {
				for (int j = 0; j < simulation.ynumberGeneral + 1; ++j) {
					for (int k = 0; k < simulation.znumberGeneral + 1; ++k) {
						if (i > simulation.firstAbsoluteXindex + simulation.xnumber) {
							break;
						}
						double x;
						double y;
						double z;
						fscanf(Efile, "%lf %lf %lf", &x, &y, &z);
						if (i >= simulation.firstAbsoluteXindex && i <= simulation.firstAbsoluteXindex + simulation.xnumber) {
							simulation.Efield[i][j][k] = Vector3d(x, y, z);
						}
					}
				}
			}
			fclose(Efile);
		}
	}
}

void readParticles(const char* particlesFileName, Simulation& simulation) {
	int size;
	int rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	for (int procCount = 0; procCount < size; ++procCount) {
		MPI_Barrier(MPI_COMM_WORLD);
		if (procCount == rank) {
			FILE* particlesFile = fopen(particlesFileName, "r");
			for (int i = 0; i < simulation.particlesNumber; ++i) {
				int number = 0;
				double mass = 0;
				double charge = 0;
				double weight = 0;
				int type;
				double x;
				double y;
				double z;
				double px;
				double py;
				double pz;
				double dx;
				double dy;
				double dz;

				fscanf(particlesFile, "%d %lf %lf %lf %d %lf %lf %lf %lf %lf %lf %lf %lf %lf", &number, &mass, &charge, &weight,
				       &type, &x, &y, &z, &px, &py, &pz, &dx, &dy, &dz);

				if (x > simulation.leftX && x <= simulation.rightX) {

					ParticleTypes particleType;
					if (type == 0) {
						particleType = ELECTRON;
					} else if (type == 1) {
						particleType = PROTON;
					} else if (type == 2) {
						particleType = POSITRON;
					} else if (type == 3) {
						particleType = ALPHA;
					} else if (type == 4) {
						particleType = DEUTERIUM;
					} else if (type == 5) {
						particleType = HELIUM3;
					} else {
						printf("particle type must be 0 1 2 3 4 5\n");
						MPI_Finalize();
						exit(0);
					}
					ParticleTypeContainer typeContainer = simulation.types[type - 1];

					//todo speed_of_light
					//todo deltaT!!!!
					Particle* particle = new Particle(number, mass, typeContainer.chargeCount, typeContainer.charge, weight,
					                                  particleType, x, y, z, px, py, pz, dx, dy, dz, 1.0);
					simulation.chargeBalance += particle->chargeCount;
					simulation.particles.push_back(particle);
				} else if (x > simulation.xgrid[simulation.xnumber]) {
					break;
				}
			}
			fclose(particlesFile);
		}
	}
}

int** readTrackedParticlesNumbers(const char* particlesFile, int& number) {
	FILE* inputFile = fopen(particlesFile, "r");
	char c;
	number = 0;
	int flag = fscanf(inputFile, "%c", &c);
	while (flag > 0) {
		if (c == '\n') {
			number++;
		}
		flag = fscanf(inputFile, "%c", &c);
	}
	rewind(inputFile);
	if (number == 0) {
		return NULL;
	}
	int** particlesNumbers = new int*[number];
	for (int i = 0; i < number; ++i) {
		particlesNumbers[i] = new int[2];
		int particleNumber;
		double x, y, z, px, py, pz, p;
		char ch = ' ';
		int type;
		//fscanf(inputFile, "%d %lf %lf %lf %lf %lf %lf", &particleNumber, &x, &y, &z, &px, &py, &pz);
		fscanf(inputFile, "%d %lf %d", &particleNumber, &p, &type);
		particlesNumbers[i][0] = particleNumber;
		particlesNumbers[i][1] = type;
	}
	fclose(inputFile);

	return particlesNumbers;
}

