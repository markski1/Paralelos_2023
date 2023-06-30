#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <omp.h>
#include <stdbool.h>

#include "funcs_base.h"

int main(int argc, char * argv[]) {
	if (argc > 5 || argc < 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0) {
		printf("Proveer N, blocksize (opcionalmente NUM_THREADS y un '1' para skippear comparación) en args.\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);

	bool comparar = true;

	// Si se especifica un 3er parametro, se usa como numero de threads.
	// Caso contrario se asume lo que diga el sistema (por ejemplo el script sbatch)
	if (argc >= 4) {
		omp_set_num_threads(atoi(argv[3]));
		// Si se especificara un '1' extra, no se compara con sec.
		if (argc == 5 && atoi(argv[4]) == 1) comparar = false;
	}

	int espaciosMatriz = N * N;

	if (N % BS != 0) {
		printf("N debe ser multiplo de bs.\n");
		return 1;
	}

	printf(COLOR_BLUE "Inicializando para:"COLOR_RESET" N=%i ; BS=%i\n", N, BS);

	// declararaciones
	double *A, *B, *C, *R, *CD, *DP2;
	int    *D, *DT2;

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
	DT2 = (int *)    malloc(sizeof(int)    *      41       ); // cache con los posibles valores de D²
	DP2 = (double *) malloc(sizeof(double) * espaciosMatriz); // cache d^2 ; + 1 espacio para evitar overflow

	// asignaciones

	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40

		A[i] = B[i] = C[i] = 1.0;
		R[i] = 0.0;
	}

	// cachear valores entre 0 y 40 de ^2
	for (int i = 0; i < 41; i++)
	{
		DT2[i] = i * i;
	}

	double tickComienzo, tickFin, escalar;

	int iPos, jPos;

	printf("Listo.\n" COLOR_BLUE "Comienza operación..." COLOR_RESET);

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	#pragma omp parallel
	{
		// Cachear DP2
		#pragma omp for private(i) schedule(static) nowait
		for (i = 0; i < espaciosMatriz; i++)
		{
			DP2[i] = DT2[D[i]]; // donde DT2 era el cache de resultados indexado por i
		}

		// sacar max, min y prom
		// se toman todos los elementos por igual, asi que no importa seguir los ordenes
		#pragma omp for private(i) reduction(+: TotalA, TotalB) reduction(min: MinA, MinB) reduction(max: MaxA, MaxB)
		for (i = 0; i < espaciosMatriz; ++i) {
			TotalA += A[i];
			if (A[i] > MaxA) MaxA = A[i];
			if (A[i] < MinA) MinA = A[i];

			TotalB += B[i];
			if (B[i] > MaxB) MaxB = B[i];
			if (B[i] < MinB) MinB = B[i];
		}
		// barrera implicita
	
		#pragma omp single
		{
			PromA = TotalA / espaciosMatriz;
			PromB = TotalB / espaciosMatriz;

			// Aca quedara cacheado el escalar
			escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);
		}
		// barrera implicita

		// COMIENZA MULTIPLICACIÓN

		// Paso 1: Multiplicar A * B, guardar en R.
		#pragma omp for private(i, j, k, iPos, jPos) schedule(static) nowait
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
		// arriba se especifica nowait, ya que la multiplicación del escalar se hara sobre la región de R indicada.

		// Paso 2: Multiplica R por el escalar.
		#pragma omp for private(i) schedule(static) nowait
		for (i = 0; i < espaciosMatriz; ++i) {
			R[i] = R[i] * escalar;
		}
		// arriba se especifica nowait, ya que lo que se computa abajo se distribuye tambien estaticamente.

		// Paso 3: Multiplicar C * Pot2(D); sumar a R
		#pragma omp for private(i, j, k, iPos, jPos) schedule(static)
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
		// aca si se espera, ya que es el fin de la operación.
	}
	
	tickFin = dwalltime();

	printf("\n==============\n"COLOR_BLUE"Finaliza operación."COLOR_RESET" Tiempo: %.5lf \n===========\nGenerando y comparando con versión secuencial...\n", tickFin - tickComienzo);

	if (comparar == false) {
		printf("==============\nPor pedido del usuario, se salta la comprobación.\n");
		return 0;
	}

	// y generar un R en R2 con el secuencial
	// se usará para comparar.
	
	double *R2 = (double *) malloc(sizeof(double) * espaciosMatriz);

	SecuencialEnRDada(N, BS, A, B, C, DP2, R2);

	bool error = false;

	for (i = 0; i < espaciosMatriz; ++i) {
		if (R[i] != R2[i]) {
			printf("==============\nERROR EN POSICION %i: R openmp: %lf ; R secuencial: %lf \n", i, R[i], R2[i]);
			error = true;
			break;
		}
	}

	if (!error) {
		printf("==============\n" COLOR_VERDE "Exito, los valores son iguales a los del secuencial.\n" COLOR_RESET);
	}
	else {
		printf("==============\n" COLOR_ROJO "Error, los valores no son iguales a los del secuencial.\n" COLOR_RESET);
	}

	return 0;
}