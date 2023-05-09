#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#include "funcs_base.h"

void * hiloMedicion(void *ptr);

int *A;
int N, NUM_THREADS, trabajoHilo, prom, min, max;

sem_t sem;

int main(int argc, char *argv[]) {
	if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N y Threads en args.\n");
		return 1;
	}

	N = atoi(argv[1]);
	NUM_THREADS = atoi(argv[2]);

	trabajoHilo = N / NUM_THREADS;
	
	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];

	pthread_attr_init(&attr);
	sem_init(&sem, 1, 1);

	int i;
	prom = 0;
	min = INT_MAX;
	max = INT_MIN;

	// alocaciones
	A = (int *) malloc(sizeof(int) * N);

	for (i = 0; i < N; ++i) {
		A[i] = (rand() % 100) + 1;
	}

	double tickComienzo, tickFin;

	int ids[NUM_THREADS];

	tickComienzo = dwalltime();

	for (i = 0; i < NUM_THREADS; ++i) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, hiloMedicion, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}

	prom = prom / N;

	tickFin = dwalltime();

	printf("min: %i, max: %i, prom: %i  ;;;  Tiempo: %.5lf \n", min, max, prom,  tickFin - tickComienzo);
}

void * hiloMedicion(void *ptr) {
	int id;
	id = * ((int *) ptr);

	int start = id * trabajoHilo;
	int end = start + trabajoHilo;

	int totalLocal = 0;
	int minLocal = INT_MAX;
	int maxLocal = INT_MIN;

	for (int i = start; i < end; ++i) {
		if (A[i] < minLocal) minLocal = A[i];
		if (A[i] > maxLocal) maxLocal = A[i];
		totalLocal += A[i];
	}

	sem_wait(&sem);
	if (minLocal < min) min = minLocal;
	if (maxLocal > max) max = maxLocal;
	prom += totalLocal;
	sem_post(&sem);
}
