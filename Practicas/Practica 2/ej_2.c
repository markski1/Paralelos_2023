#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "funcs_base.h"

void * hiloContar(void *ptr);

int *A;
int N, NUM_THREADS, trabajoHilo, X, ocurrencias;

pthread_mutex_t OM;

int main(int argc, char *argv[]) {
	if (argc != 4 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N, Threads y X en args.\n");
		return 1;
	}

	N = atoi(argv[1]);
	NUM_THREADS = atoi(argv[2]);
	X = atoi(argv[3]);

	trabajoHilo = N / NUM_THREADS;
	
	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];

	pthread_attr_init(&attr);
	pthread_mutex_init(&OM, NULL);


	int i;
	ocurrencias = 0;

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
		pthread_create(&threads[i], NULL, hiloContar, &ids[i]);
	}

	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}

	tickFin = dwalltime();

	printf("Ocurrencias: %i ; Tiempo: %.5lf \n", ocurrencias,  tickFin - tickComienzo);
}

void * hiloContar(void *ptr) {
	int id;
	id = * ((int *) ptr);

	int start = id * trabajoHilo;
	int end = start + trabajoHilo;

	for (int i = start; i < end; ++i) {
		if (A[i] == X) {
			pthread_mutex_lock(&OM);
			ocurrencias++;
			pthread_mutex_unlock(&OM);
		}
	}
}
