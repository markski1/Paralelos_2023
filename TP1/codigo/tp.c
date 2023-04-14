#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double dwalltime();
void blkmm_parte1(double *ablk, double *bblk, double *cblk, int cachedOp, int n, int bs);
void blkmm_parte2(double *ablk, int *bblk, double *cblk, int n, int bs);
void blkmul_int(int *ablk, int *bblk, int *cblk, int n, int bs);

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
	double *A, *B, *C, *R;

	int    *D, *DP2;

	double PromA, PromB = 0.0;
	double MaxA, MaxB = DBL_MIN;
	double MinA, MinB = DBL_MAX;

	double TotalA, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes

	// alocaciones
	A =   (double *)  malloc(sizeof(double) * espaciosMatriz);
	B =   (double *)  malloc(sizeof(double) * espaciosMatriz);
	C =   (double *)  malloc(sizeof(double) * espaciosMatriz);
	D =   (int *)     malloc(sizeof(double) * espaciosMatriz);
	DP2 = (int *)     malloc(sizeof(double) * espaciosMatriz); // cache d^2
	R =   (double *)  malloc(sizeof(double) * espaciosMatriz);

	// TODO: INICIAR TIMER

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40

		A[i] = B[i] = C[i] = 1.0;

		DP2[i] = 0;
		R[i] = 0.0;
	}

	double tickComienzo, tickFin, op_MinMaxProm;

	int iPos, jPos;

	printf("Inicializado. Comienza operación.\n");

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		else if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		else if (B[i] < MinB) MinA = B[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	op_MinMaxProm = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	// COMIENZA MULTIPLICACIÓN

	// Primero cachear todo D²
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{			
			for (k = 0; k < N; k += BS)
			{
				//                          j solo se usa una vez asi que parece contraintuitivo cachearlo
				blkmul_int(&D[iPos + k], &D[j*N + k], &DP2[iPos + j], N, BS);
			}
		}
	}

	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j + N;
			for (k = 0; k < N; k += BS)
			{
				// blkmm_parte1 guardara en R el resultado de Escalar * [A * B].
				blkmm_parte1(&A[iPos + k], &B[jPos + k], &R[iPos + j], op_MinMaxProm, N, BS);
				// blkmm_parte2 calculara el resultado de C * D² y se lo sumara a R (el cual ya contiene Escalar * [A * B]), completando la operación.
				blkmm_parte2(&C[iPos + k], &DP2[jPos + j], &R[iPos + j], N, BS);
			}
		}
	}

	tickFin = dwalltime();

	printf("Finaliza operación. Tiempo: %.5lf \n", tickFin - tickComienzo);

	return 0;
}

// COMIENZA FUNCIONES PARA MULTIPLICACIÓN POR BLOQUE

void blkmm_parte1(double *ablk, double *bblk, double *cblk, int cachedOp, int n, int bs)
{
	int i, j, k,
	    iPos;

	for (i = 0; i < bs; i++)
	{
		iPos = i*n;
		for (j = 0; j < bs; j++)
		{
			for (k = 0; k < bs; k++)
			{
				cblk[iPos + j] += ablk[iPos + k] * bblk[j*n + k];
			}
			// una vez arriba hecho A * B, multiplicarle el escalar
			cblk[iPos + j] *= cachedOp;
		}
	}
}

void blkmm_parte2(double *ablk, int *bblk, double *cblk, int n, int bs)
{
	int i, j, k,
	    iPos;

	for (i = 0; i < bs; i++)
	{
		iPos = i*n;
		for (j = 0; j < bs; j++)
		{
			for (k = 0; k < bs; k++)
			{
				cblk[iPos + j] += ablk[iPos + k] * bblk[j*n + k];
			}
		}
	}
}

void blkmul_int(int *ablk, int *bblk, int *cblk, int n, int bs)
{
	int i, j, k,
	    iPos;

	for (i = 0; i < bs; i++)
	{
		iPos = i*n;
		for (j = 0; j < bs; j++)
		{
			for (k = 0; k < bs; k++)
			{
				cblk[iPos + j] += ablk[iPos + k] * bblk[j*n + k];
			}
		}
	}
}
    
/*****************************************************************/

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

/*

- El escalar debe primero multipliarse por A x B y despues eso se suma a C * D².
- D se debe cachear de otra manera.

*/