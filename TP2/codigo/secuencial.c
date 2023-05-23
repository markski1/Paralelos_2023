#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "funcs_base.h"

int main(int argc, char * argv[]) {
	if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N y blocksize en args.\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);
	int espaciosMatriz = N * N;

	if (N % BS != 0) {
		printf("N debe ser multiplo de bs.\n");
		return 1;
	}

	printf("Inicializando para operación con matrices de %ix%i ; block size de %i \n", N, N, BS);

	// declararaciones
	double *A, *B, *C, *R, *CD, *DP2;
	int    *D;

	double PromA, PromB;
	double MaxA = DBL_MIN, MaxB = DBL_MIN;
	double MinA = DBL_MIN, MinB = DBL_MAX;

	double TotalA = 0.0, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes

	// alocaciones
	A   = (double *) malloc(sizeof(double) * espaciosMatriz);
	B   = (double *) malloc(sizeof(double) * espaciosMatriz);
	C   = (double *) malloc(sizeof(double) * espaciosMatriz);
	R   = (double *) malloc(sizeof(double) * espaciosMatriz);
	D   = (int *)    malloc(sizeof(int)    * espaciosMatriz);
	DP2 = (double *) malloc(sizeof(double) * espaciosMatriz); // cache d^2 ; + 1 espacio para evitar overflow

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40
		DP2[i] = D[i] * D[i];

		A[i] = B[i] = C[i] = 1.0;
		R[i] = 0.0;
	}

	double tickComienzo, tickFin, escalar;

	int iPos, jPos;

	printf("Listo.\nComienza operación...\n");

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		if (B[i] < MinB) MinB = B[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	// COMIENZA MULTIPLICACIÓN

	// Paso 1: Multiplicar A * B, guardar en R.
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&A[iPos + k], &B[jPos + k], &R[iPos + j], N, BS);
			}
		}
	}

	// Paso 2: Multiplica R por el escalar.
	for (i = 0; i < espaciosMatriz; ++i) {
		R[i] = R[i] * escalar;
	}

	// Paso 3: Multiplicar C * Pot2(D); sumar a R
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&C[iPos + k], &DP2[jPos + k], &R[iPos + j], N, BS);
			}
		}
	}
	

	tickFin = dwalltime();

	printf("\n==============\nFinaliza operación. Tiempo: %.5lf \n", tickFin - tickComienzo);

	return 0;
}