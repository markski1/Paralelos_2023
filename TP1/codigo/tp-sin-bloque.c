#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double dwalltime();

int main(int argc, char * argv[]) {
	if (argc != 2 || atoi(argv[1]) <= 0) {
		printf("Proveer N en args.");
		return 1;
	}

	int N = atoi(argv[1]);
	int espaciosMatriz = N * N;

	// declararaciones
	double *A, *B, *C, *R;
	int *D, *DP2;

	double PromA, PromB = 0.0;
	double MaxA, MaxB = DBL_MIN;
	double MinA, MinB = DBL_MAX;

	double TotalA, TotalB = 0.0; // para sacar promedio

	int i, j;

	// alocaciones
	A =    (double *) malloc(sizeof(double) * espaciosMatriz);
	B =    (double *) malloc(sizeof(double) * espaciosMatriz); // columnas
	C =    (double *) malloc(sizeof(double) * espaciosMatriz);
	R =    (double *) malloc(sizeof(double) * espaciosMatriz);
	D =    (int *)    malloc(sizeof(double) * espaciosMatriz); // columnas
	DP2 =  (int *)    malloc(sizeof(double) * espaciosMatriz); // cache d^2

	// TODO: INICIAR TIMER

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // deben ser valores entre 1 y 40
		A[i] = B[i] = C[i] = 1.0;

		R[i] = 0.0;
	}

	double tickComienzo, tickFin, op_MinMaxProm;

	// cachear posiciones
	int iPos, jPos;

	tickComienzo = dwalltime();

	// sacar max, min y prom, y cachear D^2
	
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		else if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		else if (B[i] < MinB) MinA = B[i];

		DP2[i] = D[i] * D[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	op_MinMaxProm = (MaxA * MaxB - MinA * MinB) / PromA * PromB;

	for (i = 0; i < N; ++i) {
		iPos = i * N;
		for (j = 0; j < N; ++j) {
			jPos = j * N;
			for (int k = 0; k < N; ++k) {
				R[iPos+j] += op_MinMaxProm * (A[iPos+k] * B[k+jPos]) + (C[iPos+k] * DP2[k+jPos]);
			}
		}
	}

	tickFin = dwalltime();

	printf("Tiempo de ejecución: %.5lf \n", tickFin - tickComienzo);

	return 0;
}

// COMIENZA HELPER PARA CALCULAR TIEMPO

#include <sys/time.h>

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
