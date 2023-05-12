#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <omp.h>

#include "funcs_base.h"

int main(int argc, char * argv[]) {
	if (argc != 4 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 || atoi(argv[3]) <= 0) {
		printf("Proveer N, blocksize y NUM_THREADS en args.\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);
	omp_set_num_threads(atoi(argv[3]));
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
	DP2 = (double *) malloc(sizeof(double) *       41      ); // cache d^2 ; + 1 espacio para evitar overflow

	// asignaciones

	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40

		A[i] = B[i] = C[i] = 1.0;
		R[i] = 0.0;
	}

	// Cachear un arreglo con pow2 D
	// pueden ser de 1 a 40, y C es zero-indexed, asi que...
	for (i = 0; i < 41; ++i) {
		DP2[i] = (double) (i * i);
	}

	double tickComienzo, tickFin, escalar;

	int iPos, jPos;

	printf("Listo.\nComienza operación...\n");

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	#pragma omp parallel
	{
		// sacar max, min y prom
		// se toman todos los elementos por igual, asi que no importa seguir los ordenes
		#pragma omp for private(i) reduction(+: TotalA, TotalB)
		for (i = 0; i < espaciosMatriz; ++i) {
			TotalA += A[i];
			if (A[i] > MaxA) MaxA = A[i];
			if (A[i] < MinA) MinA = A[i];

			TotalB += B[i];
			if (B[i] > MaxB) MaxB = B[i];
			if (B[i] < MinB) MinB = B[i];
		}
	
		#pragma omp single
		{
			PromA = TotalA / espaciosMatriz;
			PromB = TotalB / espaciosMatriz;

			// Aca quedara cacheado el escalar
			escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);
		}

		// COMIENZA MULTIPLICACIÓN

		// Paso 1: Multiplicar A * B, guardar en R.
		#pragma omp for private(i, j, k, iPos, jPos) nowait
		for (i = 0; i < N; i += BS)
		{
			iPos = i * N;
			for (j = 0; j < N; j += BS)
			{
				jPos = j * N;
				for (k = 0; k < N; k += BS)
				{
					blkmm_parte1(&A[iPos + k], &B[jPos + k], &R[iPos + j], N, BS);
				}
			}
		}
		// arriba se especifica nowait, ya que la multiplicación del escalar se hara sobre la región de R indicada.

		// Paso 2: Multiplica R por el escalar.
		#pragma omp for private(i) schedule(static, BS) nowait
		for (i = 0; i < espaciosMatriz; ++i) {
			R[i] = R[i] * escalar;
		}
		// arriba se especifica nowait, ya que la multiplicación se hara, de nuevo, sobre los espacios de R ya manejados

		// Paso 3: Multiplicar C * Pot2(D); sumar a R
		#pragma omp for private(i, j, k, iPos, jPos) 
		for (i = 0; i < N; i += BS)
		{
			iPos = i * N;
			for (j = 0; j < N; j += BS)
			{
				jPos = j * N;
				for (k = 0; k < N; k += BS)
				{
					blkmm_parte2(&C[iPos + k], &D[jPos + k], &R[iPos + j], DP2, N, BS);
				}
			}
		}
		// aca si se espera, ya que es el fin de la operación.
	}
	
	tickFin = dwalltime();

	printf("Finaliza operación. Tiempo: %.5lf \n", tickFin - tickComienzo);

	return 0;
}