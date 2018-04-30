//#include <crtdbg.h>
#include "stdlib.h"
#include <cmath>

//#include "memory_debug.h"
#include "boundaryFieldEvaluator.h"
#include "paths.h"
#include "constants.h"
#include "rightPartEvaluator.h"
#include "random.h"
#include "simulation.h"

BoundaryFieldEvaluator::BoundaryFieldEvaluator() {

}

BoundaryFieldEvaluator::~BoundaryFieldEvaluator() {

}

void BoundaryFieldEvaluator::prepareB(double t) {

}

void BoundaryFieldEvaluator::prepareE(double t) {

}


ConstantBoundaryFieldEvaluator::ConstantBoundaryFieldEvaluator(Vector3d& E0, Vector3d& B0) {
	E = E0;
	B = B0;
}

Vector3d ConstantBoundaryFieldEvaluator::evaluateEfield(double t, int j, int k) {
	return E;
}

Vector3d ConstantBoundaryFieldEvaluator::evaluateBfield(double t, int j, int k) {
	return B;
}

int StripeBoundaryFieldEvaluator::halfPeriodNumber(double& t) {
	return ((t * ux * 2 + shiftX) / lambda);
}

StripeBoundaryFieldEvaluator::
StripeBoundaryFieldEvaluator(Vector3d& E0, Vector3d& B0, double l, double u, double shift) {
	E = E0;
	B = B0;
	lambda = 0;
	ux = ux;
	shiftX = shift;
}

Vector3d StripeBoundaryFieldEvaluator::evaluateEfield(double t, int j, int k) {
	int n = halfPeriodNumber(t);
	return E * n;
}

Vector3d StripeBoundaryFieldEvaluator::evaluateBfield(double t, int j, int k) {
	int n = halfPeriodNumber(t);
	return B * n;
}

TurbulenceBoundaryFieldEvaluator::TurbulenceBoundaryFieldEvaluator(Vector3d& E, Vector3d& B, Vector3d& V, int numberv,
                                                                   double* amplitudev,
                                                                   double* phasev, double* kv, double* omegav,
                                                                   double xv, double c) {
	E0 = E;
	B0 = B;
	V0 = V;
	speed_of_light_normalized = c;
	x = xv;
	number = numberv;
	phase = new double[2 * number];
	amplitude = new double[2 * number];
	kw = new double[number];
	omega = new double[number];
	for (int i = 0; i < number; ++i) {
		kw[i] = kv[i];
		omega[i] = omegav[i];
	}
	for (int i = 0; i < 2 * number; ++i) {
		phase[i] = phasev[i];
		amplitude[i] = amplitudev[i];
	}
}

TurbulenceBoundaryFieldEvaluator::~TurbulenceBoundaryFieldEvaluator() {
	delete[] phase;
	delete[] amplitude;
	delete[] kw;
	delete[] omega;
}

Vector3d TurbulenceBoundaryFieldEvaluator::evaluateBfield(double t, int j, int k) {
	Vector3d result = B0;
	for (int i = 0; i < number; ++i) {
		result.y += amplitude[2 * i] * sin(kw[i] * (x - V0.x * t) - omega[i] * t + phase[2 * i]);
		result.z += amplitude[2 * i + 1] * sin(kw[i] * (x - V0.x * t) - omega[i] * t + phase[2 * i + 1]);
	}
	return result;
}

Vector3d TurbulenceBoundaryFieldEvaluator::evaluateEfield(double t, int j, int k) {
	Vector3d result = E0;
	Vector3d B = evaluateBfield(t, j, k) - B0;
	result = result - V0.vectorMult(B) / (speed_of_light_normalized * speed_of_light_correction);
	return result;
}

RandomTurbulenceBoundaryFieldEvaluator::RandomTurbulenceBoundaryFieldEvaluator(
	int randomSeedV, int minLengthXV, int maxLengthXV, int minLengthYV,
	int maxLengthYV, int minLengthZV, int maxLengthZV,
	Simulation* simuationV, Vector3d V0, Vector3d E, Vector3d B, double xv,
	double dx, double dy, double dz, int xnumberGeneralV, int ynumberGeneralV, int znumberGeneralV, int xnumberAddedV,
	int ynumberAddedV, int znumberAddedV) {
	randomSeed = randomSeedV;
	minLengthX = minLengthXV;
	maxLengthX = maxLengthXV;
	minLengthY = minLengthYV;
	maxLengthY = maxLengthYV;
	minLengthZ = minLengthZV;
	maxLengthZ = maxLengthZV;
	simulation = simuationV;
	V = V0;
	E0 = E;
	B0 = B;
	x = xv;
	deltaX = dx;
	deltaY = dy;
	deltaZ = dz;
	xnumberGeneral = xnumberGeneralV;
	ynumberGeneral = ynumberGeneralV;
	znumberGeneral = znumberGeneralV;
	xnumberAdded = xnumberAddedV;
	ynumberAdded = ynumberAddedV;
	znumberAdded = znumberAddedV;

	Efield = new Vector3d*[ynumberAdded + 1];
	for (int j = 0; j < ynumberAdded + 1; ++j) {
		Efield[j] = new Vector3d[znumberAdded + 1];
		for (int k = 0; k < znumberAdded + 1; ++k) {
			Efield[j][k] = E0;
		}
	}

	Bfield = new Vector3d*[ynumberAdded];
	for (int j = 0; j < ynumberAdded; ++j) {
		Bfield[j] = new Vector3d[znumberAdded];
		for (int k = 0; k < znumberAdded; ++k) {
			Bfield[j][k] = B0;
		}
	}
}

RandomTurbulenceBoundaryFieldEvaluator::~RandomTurbulenceBoundaryFieldEvaluator() {
	for(int j = 0; j < ynumberAdded + 1; ++j) {
		delete[] Efield[j];
	}
	delete[] Efield;
	for(int j = 0; j < ynumberAdded +  1; ++j) {
		delete[] Bfield[j];
	}
	delete[] Bfield;
}


Vector3d RandomTurbulenceBoundaryFieldEvaluator::evaluateBfield(double t, int j, int k) {
	return Bfield[j][k];
}

void RandomTurbulenceBoundaryFieldEvaluator::prepareB(double t) {
	srand(randomSeed);
	int maxKxnumber = maxLengthX / minLengthX;
	int maxKynumber = maxLengthY / minLengthY;
	int maxKznumber = maxLengthZ / minLengthZ;

	if (ynumberGeneral == 1) {
		maxKynumber = 1;
	}
	if (znumberGeneral == 1) {
		maxKznumber = 1;
	}

	for(int j = 0; j < ynumberAdded; ++j) {
		for(int k = 0; k < znumberAdded; ++k) {
			Bfield[j][k] = B0;
		}
	}
	for (int ki = 0; ki < maxKxnumber; ki++) {
		double kx = 2 * pi * ki / (deltaX * maxLengthX);
		for (int kj = 0; kj < maxKynumber; ++kj) {
			double ky = 2 * pi * kj / (deltaY * maxLengthY);
			for (int kk = 0; kk < maxKznumber; ++kk) {
				if (ki + kj + kk > 0) {
					double kz = 2 * pi * kk / (deltaZ * maxLengthZ);
					double phase1 = 2 * pi * uniformDistribution();
					double phase2 = 2 * pi * uniformDistribution();

					double kw = sqrt(kx * kx + ky * ky + kz * kz);
					double kyz = sqrt(ky * ky + kz * kz);
					double cosTheta = kx / kw;
					double sinTheta = kyz / kw;
					double cosPhi;
					double sinPhi;
					if (kk + kj > 0) {
						cosPhi = ky / kyz;
						sinPhi = kz / kyz;
					}
					else {
						cosPhi = 1.0;
						sinPhi = 0.0;
					}
					double Bturbulent = simulation->evaluateTurbulenceFieldAmplitude(kx, ky, kz);
					for(int j = 1 + additionalBinNumber; j < ynumberAdded - 1 - additionalBinNumber; ++j) {
						for(int k = 1 + additionalBinNumber; k < znumberAdded -1 - additionalBinNumber; ++k){
							double kmultr = kx * (x - 0.5 * deltaX - V.x * t) + ky * (simulation->middleYgrid[j] - V.y * t) + kz * (simulation->middleZgrid[k] - V.z * t);
							double localB1 = Bturbulent * sin(kmultr + phase1);
							double localB2 = Bturbulent * sin(kmultr + phase2);
							Vector3d localB = Vector3d(-sinTheta * localB1, cosTheta * cosPhi * localB1 - sinTheta * localB2,
					                           cosTheta * sinPhi * localB1 - cosTheta * localB2);
							Bfield[j][k] = Bfield[j][k] + localB;
						}
					}
				}
			}
		}
	}
}

void RandomTurbulenceBoundaryFieldEvaluator::prepareE(double t) {
	srand(randomSeed);
	int maxKxnumber = maxLengthX / minLengthX;
	int maxKynumber = maxLengthY / minLengthY;
	int maxKznumber = maxLengthZ / minLengthZ;

	if (ynumberGeneral == 1) {
		maxKynumber = 1;
	}
	if (znumberGeneral == 1) {
		maxKznumber = 1;
	}

	for(int j = 0; j < ynumberAdded + 1; ++j) {
		for(int k = 0; k < znumberAdded + 1; ++k) {
			Efield[j][k] = E0;
		}
	}
	for (int ki = 0; ki < maxKxnumber; ki++) {
		double kx = 2 * pi * ki / (deltaX * maxLengthX);
		for (int kj = 0; kj < maxKynumber; ++kj) {
			double ky = 2 * pi * kj / (deltaY * maxLengthY);
			for (int kk = 0; kk < maxKznumber; ++kk) {
				if (ki + kj + kk > 0) {
					double kz = 2 * pi * kk / (deltaZ * maxLengthZ);
					double phase1 = 2 * pi * uniformDistribution();
					double phase2 = 2 * pi * uniformDistribution();

					double kw = sqrt(kx * kx + ky * ky + kz * kz);
					double kyz = sqrt(ky * ky + kz * kz);
					double cosTheta = kx / kw;
					double sinTheta = kyz / kw;
					double cosPhi;
					double sinPhi;
					if (kk + kj > 0) {
						cosPhi = ky / kyz;
						sinPhi = kz / kyz;
					}
					else {
						cosPhi = 1.0;
						sinPhi = 0.0;
					}

					double Bturbulent = simulation->evaluateTurbulenceFieldAmplitude(kx, ky, kz);

					for(int j = 1 + additionalBinNumber; j < ynumberAdded - additionalBinNumber; ++j) {
						for(int k = 1 + additionalBinNumber; k < znumberAdded - additionalBinNumber; ++k) {
							double kmultr = kx * (x - V.x * t) + ky * (simulation->ygrid[j] - V.y * t) + kz * (simulation->zgrid[k] - V.z * t);
							double localB1 = Bturbulent * sin(kmultr + phase1);
							double localB2 = Bturbulent * sin(kmultr + phase2);
							Vector3d localB = Vector3d(-sinTheta * localB1, cosTheta * cosPhi * localB1 - sinTheta * localB2,
					                           cosTheta * sinPhi * localB1 - cosTheta * localB2);
							Efield[j][k] = Efield[j][k] - V.vectorMult(localB) / (simulation->speed_of_light_normalized);
						}
					}
				}
			}
		}
	}
}

Vector3d RandomTurbulenceBoundaryFieldEvaluator::evaluateEfield(double t, int j, int k) {
	return Efield[j][k];
}













