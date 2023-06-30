#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <pthread.h>
#include <stdbool.h>

#include "funcs_base.h"

int N, BS, NUM_THREADS, espaciosMatriz;

double *A, *B, *C, *R, *CD, *DP2;
int    *D, *DT2;

double PromA, PromB;
double MaxA = DBL_MIN, MaxB = DBL_MIN;
double MinA = DBL_MIN, MinB = DBL_MAX;

double TotalA = 0.0, TotalB = 0.0;

double escalar;

pthread_mutex_t MTX;
pthread_barrier_t BAR;

void * hiloOperacion(void *ptr);

int main(int argc, char * argv[]) {
	if (argc > 5 || argc < 4 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N, blocksize y NUM_THREADS en args.\n");
		return 1;
	}

	N = atoi(argv[1]);
	BS = atoi(argv[2]);
	NUM_THREADS = atoi(argv[3]);
	espaciosMatriz = N * N;

	if (N % BS != 0) {
		printf("N debe ser multiplo de bs.\n");
		return 1;
	}

	bool comparar = true;
	// Si se especificara un '1' extra, no se compara con sec.
	if (argc == 5 && atoi(argv[4]) == 1) comparar = false;

	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];

	pthread_attr_init(&attr);
	pthread_mutex_init(&MTX, NULL);
	pthread_barrier_init(&BAR, NULL, NUM_THREADS);

	printf(COLOR_BLUE "Inicializando para:"COLOR_RESET" N=%i ; BS=%i\n", N, BS);

	// alocaciones
	A   = (double *) malloc(sizeof(double) * espaciosMatriz);
	B   = (double *) malloc(sizeof(double) * espaciosMatriz);
	C   = (double *) malloc(sizeof(double) * espaciosMatriz);
	R   = (double *) malloc(sizeof(double) * espaciosMatriz);
	D   = (int *)    malloc(sizeof(int)    * espaciosMatriz);
	DT2 = (int *)    malloc(sizeof(int)    *      41       ); // cache con los posibles valores de D²
	DP2 = (double *) malloc(sizeof(double) * espaciosMatriz); // cache d^2 ; + 1 espacio para evitar overflow

	int    i;

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


	double tickComienzo, tickFin;

	int ids[NUM_THREADS];

	printf("Listo.\n" COLOR_BLUE "Comienza operación..." COLOR_RESET);

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	for (i = 0; i < NUM_THREADS; ++i) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, hiloOperacion, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
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
			printf("==============\nERROR EN POSICION %i: R pthread: %lf ; R secuencial: %lf \n", i, R[i], R2[i]);
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

void * hiloOperacion(void *ptr) {
	int id;
	id = * ((int *) ptr);

	// variables para operaciones
	int i, j, k, iPos, jPos;

	// La "responsabilidad" de cada hilo sera el espacio de memoria en el cual trabajara.
	int responsabilidad = (espaciosMatriz / NUM_THREADS);
	int start = id * responsabilidad;
	int end = start + responsabilidad;

	// Cada hilo llevara su cuenta local.
	double MaxAlocal = DBL_MIN, MaxBlocal = DBL_MIN;
	double MinAlocal = DBL_MIN, MinBlocal = DBL_MAX;

	double TotalAlocal = 0.0, TotalBlocal = 0.0;

	for (i = start; i < end; i++)
	{
		DP2[i] = DT2[D[i]]; // donde DT2 era el cache de resultados indexado por i
	}

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = start; i < end; ++i) {
		TotalAlocal += A[i];
		if (A[i] > MaxAlocal) MaxAlocal = A[i];
		if (A[i] < MinAlocal) MinAlocal = A[i];

		TotalBlocal += B[i];
		if (B[i] > MaxBlocal) MaxBlocal = B[i];
		if (B[i] < MinBlocal) MinBlocal = B[i];
	}

	// Luego, dentro de este mutex, se escribe a los resultados globales.
	// Medio engorroso, pero sale mucho mas caro hacer el mutex arriba.
	// Menos sincronizacion = Menos overhead
	pthread_mutex_lock(&MTX);
	TotalA += TotalAlocal;
	TotalB += TotalBlocal;
	if (MaxAlocal > MaxA) MaxA = MaxAlocal;
	if (MaxBlocal > MaxB) MaxB = MaxBlocal;

	if (MinAlocal < MinA) MinA = MinAlocal;
	if (MinBlocal < MinB) MinB = MinBlocal;
	pthread_mutex_unlock(&MTX);

	// Este barrier para que no se calcule el promedio hasta que todos los hilos
	// hayan hecho su parte.
	pthread_barrier_wait(&BAR);

	// solo el primer thread spawnedo
	if (id == 0) {
		PromA = TotalA / espaciosMatriz;
		PromB = TotalB / espaciosMatriz;

		// Aca quedara cacheado el escalar
		escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);
	}

	// Este barrier, en teoria, para evitar que se llegue a la parte de multiplicación
	// de escalar antes de que lo de arriba este hecho.
	// En practica me parece imposible que todos los demas hilos lleguen a ese punto sin
	// que el escalar se complete, pero bueno, voy por teoria.
	pthread_barrier_wait(&BAR);
	
	// para el caso de multiplicar matrices, la responsabilidad es de N sobre numthreads
	responsabilidad = (N / NUM_THREADS);
	start = id * responsabilidad;
	end = start + responsabilidad;

	// Paso 1: Multiplicar A * B, guardar en R.
	for (i = start; i < end; i += BS)
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

	// como la multiplicacion al escalar lo hacemos con R en linea estilo vector,
	// dividimos la responsabilidad distinto.
	int responsabilidadREscalar = (espaciosMatriz / NUM_THREADS);
	int startRE = id * responsabilidad;
	int endRE = start + responsabilidad;

	// Paso 2: Multiplica R por el escalar.
	for (i = startRE; i < endRE; ++i) {
		R[i] = R[i] * escalar;
	}

	// Paso 3: Multiplicar C * Pot2(D); sumar a R
	for (i = start; i < end; i += BS)
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
}