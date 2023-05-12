#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <float.h>

#include "funcs_base.h"

int N, BS, NUM_THREADS, espaciosMatriz;

double *A, *B, *C, *R, *CD, *DP2;
int    *D;

double PromA, PromB;
double MaxA = DBL_MIN, MaxB = DBL_MIN;
double MinA = DBL_MIN, MinB = DBL_MAX;

double TotalA = 0.0, TotalB = 0.0;

double escalar;

pthread_mutex_t MTX;

void * hiloMultiplicar(void *ptr);
void * hiloPromedios(void *ptr);


int main(int argc, char * argv[]) {
	if (argc != 4 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
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

	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];

	pthread_attr_init(&attr);
	pthread_mutex_init(&MTX, NULL);

	printf("Inicializando para operación con matrices de %ix%i ; block size de %i \n", N, N, BS);

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

	double tickComienzo, tickFin;

	int ids[NUM_THREADS];

	printf("Listo.\nComienza operación...\n");

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	for (i = 0; i < NUM_THREADS; ++i) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, hiloPromedios, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	for (i = 0; i < NUM_THREADS; ++i) {
		// ids[i] ya esta seteado de arriba
		pthread_create(&threads[i], NULL, hiloMultiplicar, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}	

	tickFin = dwalltime();

	printf("Finaliza operación. Tiempo: %.5lf \n", tickFin - tickComienzo);

	return 0;
}

void * hiloPromedios(void *ptr) {
	int id;
	id = * ((int *) ptr);

	// La "responsabilidad" de cada hilo sera el espacio de memoria en el cual trabajara.
	int responsabilidad = (espaciosMatriz / NUM_THREADS);
	int start = id * responsabilidad;
	int end = start + responsabilidad;

	// Cada hilo llevara su cuenta local.
	double MaxAlocal = DBL_MIN, MaxBlocal = DBL_MIN;
	double MinAlocal = DBL_MIN, MinBlocal = DBL_MAX;

	double TotalAlocal = 0.0, TotalBlocal = 0.0;

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (int i = start; i < end; ++i) {
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
}

void * hiloMultiplicar(void *ptr) {
	int id;
	id = * ((int *) ptr);

	int i, j, k, iPos, jPos;

	int responsabilidad = (N / NUM_THREADS);

	int start = id * responsabilidad;
	int end = start + responsabilidad;

	// Paso 1: Multiplicar A * B, guardar en R.
	for (i = start; i < end; i += BS)
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

	int responsabilidadREscalar = (espaciosMatriz / NUM_THREADS);
	
	int startME = id * responsabilidad;
	int endME = start + responsabilidad;

	// Paso 2: Multiplica R por el escalar.
	for (i = startME; i < endME; ++i) {
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
				blkmm_parte2(&C[iPos + k], &D[jPos + k], &R[iPos + j], DP2, N, BS);
			}
		}
	}
}