#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double dwalltime();
void blkmm_parte1(double *A_blk, double *B_blk, double *R_blk, int n, int bs);
void blkmm_parte2(double *C_blk, int *D_blk, double *CB_blk, double *D_2, int n, int bs);

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
	CD  = (double *) malloc(sizeof(double) * espaciosMatriz); // resultado de C*D²
	D   = (int *)    malloc(sizeof(int)    * espaciosMatriz);
	DP2 = (double *) malloc(sizeof(double) *        41      ); // cache d^2 ; + 1 espacio para evitar overflow

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

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		if (B[i] < MinB) MinA = B[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	// COMIENZA MULTIPLICACIÓN

	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				// blkmm_parte1 guardara en R el resultado de [A * B].
				blkmm_parte1(&A[iPos + k], &B[jPos + k], &R[iPos + j], N, BS);
			}
		}
	}

	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				// blkmm_parte2 calculara el resultado de C * D² y lo sumara a R, completando la operación.
				blkmm_parte2(&C[iPos + k], &D[jPos + k], &CD[iPos + j], DP2, N, BS);
			}
		}
	}

	for (i = 0; i < N; ++i) {
		R[i] = R[i] * escalar;
		R[i] = R[i] + CD[i];
	}

	tickFin = dwalltime();

	printf("Finaliza operación. Tiempo: %.5lf \n", tickFin - tickComienzo);

	return 0;
}

// COMIENZA FUNCIONES PARA MULTIPLICACIÓN POR BLOQUE

//                        A              B              R
void blkmm_parte1(double *A_blk, double *B_blk, double *R_blk, int n, int bs)
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
			R_blk[iPos + j] = registro;
		}
	}
}

//                        C           D              CD             Cache D²
void blkmm_parte2(double *C_blk, int *D_blk, double *CD_blk, double *D_2, int n, int bs)
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
				registro += C_blk[iPos + k] * D_2[ D_blk[jPos + k] ];
			}
			CD_blk[iPos + j] = registro;
		}
	}
}

// COMIENZA HELPER PARA CALCULAR TIEMPO

#include <sys/time.h>

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}