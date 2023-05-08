#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "funcs_base.h"

void * hiloMultiplicar(void *ptr);
void blkmm(double *A_blk, double *B_blk, double *C_blk, int n, int bs);

double *A, *B, *C;
int N, BS, NUM_THREADS, espaciosMatriz, filasHilo;

int main(int argc, char *argv[]) {
	if (argc != 4 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 || atoi(argv[3]) < 2 ) {
		printf("Proveer N, blocksize y T en args.\n");
		return 1;
	}

	N = atoi(argv[1]);
	BS = atoi(argv[2]);
	NUM_THREADS = atoi(argv[3]);

	filasHilo = N / NUM_THREADS;
	
	espaciosMatriz = N * N;

	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];

	pthread_attr_init(&attr);

	if (N % BS != 0) {
		printf("N debe ser multiplo de bs.\n");
		return 1;
	}

	int i;

	// alocaciones
	A = (double *) malloc(sizeof(double) * espaciosMatriz);
	B = (double *) malloc(sizeof(double) * espaciosMatriz);
	C = (double *) malloc(sizeof(double) * espaciosMatriz);


	for (i = 0; i < espaciosMatriz; ++i) {
		A[i] = B[i] = 1.0;
		C[i] = 0.0;
	}

	double tickComienzo, tickFin;

	int ids[NUM_THREADS];

	tickComienzo = dwalltime();

	for (i = 0; i < NUM_THREADS; ++i) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, hiloMultiplicar, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}

	tickFin = dwalltime();

	printf("Tiempo: %.5lf \n", tickFin - tickComienzo);
}

void * hiloMultiplicar(void *ptr) {
	int id;
	id = * ((int *) ptr);

	int start = id * filasHilo;
	int end = start + filasHilo;

	int iPos, jPos;

	int i, j, k;

	for (i = start; i < end; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&A[iPos + k], &B[jPos + k], &C[iPos + j], N, BS);
			}
		}
	}
}

//                 A              B              C
void blkmm(double *A_blk, double *B_blk, double *C_blk, int n, int bs)
{
	int i, j, k,
	    iPos, jPos;

	double registro;

	for (i = 0; i < bs; i++)
	{
		iPos = i*n;
		for (j = 0; j < bs; j++)
		{
			jPos = j*n;
			registro = 0.0;
			for (k = 0; k < bs; k++)
			{
				registro += A_blk[iPos + k] * B_blk[jPos + k];
			}
			C_blk[iPos + j] = registro;
		}
	}
}