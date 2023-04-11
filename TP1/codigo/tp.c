#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>

double dwalltime();

/**********Para calcular tiempo*************************************/
double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}
/****************************************************************/

int main(int argc, char * argv[]) {
	if (argc <= 1 || atoi(argv[1]) <= 0 ) {
		printf("Proveer N en args.");
		return 1;
	}

	int N = atoi(argv[1]);
	int NN = N * N;

	// declararaciones
	double *A, *B, *C, *R;
	int *D;

	double PromA, PromB = 0.0;
	double MaxA, MaxB = DBL_MIN;
	double MinA, MinB = DBL_MAX;

	double TotalA, TotalB = 0.0; // para sacar promedio

	int i, j;

	// alocaciones
	A = (double *) malloc(sizeof(double) * NN);
	B = (double *) malloc(sizeof(double) * NN); // columnas
	C = (double *) malloc(sizeof(double) * NN);
	R = (double *) malloc(sizeof(double) * NN);
	D = (int *)    malloc(sizeof(double) * NN); // columnas

	// TODO: INICIAR TIMER

	// asignaciones
	for (i = 0; i < NN; ++i) {
		D[i] = (rand() % 40) + 1; // deben ser valores entre 1 y 40
		A[i] = B[i] = C[i] = 1.0;
	}

	double comienzo, fin;

	comienzo = dwalltime();

	// sacar max, min y prom
	
	for (i = 0; i < NN; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		else if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		else if (B[i] < MinB) MinA = B[i];
	}

	PromA = TotalA / NN;
	PromB = TotalB / NN;

	// cachear posiciones
	int iPos, jPos;

	double opCache = (MaxA * MaxB - MinA * MinB) / PromA * PromB;

	for (i = 0; i < N; ++i) {
		iPos = i * N;
		for (j = 0; j < N; ++j) {
			jPos = j * N;
			for (int k = 0; k < N; ++k)
				R[iPos+j] = opCache * (A[iPos+k] * B[k+jPos]) + (C[iPos+k] * pow(D[k+jPos], 2));
			// (MaxA * MaxB - MinA * MinB) / PromA * PromB) 
			// * [A * B] + [C * POW2(D)]
		}
	}

	fin = dwalltime();

	printf("Tiempo de ejecución: %.10lf \n", fin - comienzo);

	return 0;
}