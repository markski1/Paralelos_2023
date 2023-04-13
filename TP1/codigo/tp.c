#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double dwalltime();
void blkmul(double *ablk, double *bblk, double *cblk, int *dblk, double *abcd_blk, int n, int bs);

int main(int argc, char * argv[]) {
	if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N y blocksize en args.");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);
	int espaciosMatriz = N * N;

	if (N % BS != 0) {
		printf("N debe ser multiplo de bs.");
		return 1;
	}

	// declararaciones
	double *A, *B, *C, *R,
	       *ABCD;

	int    *D, *DP2;

	double PromA, PromB = 0.0;
	double MaxA, MaxB = DBL_MIN;
	double MinA, MinB = DBL_MAX;

	double TotalA, TotalB = 0.0; // para sacar promedio

	int i, j, k;

	// alocaciones
	A =    (double *)  malloc(sizeof(double) * espaciosMatriz);
	B =    (double *)  malloc(sizeof(double) * espaciosMatriz);
	C =    (double *)  malloc(sizeof(double) * espaciosMatriz);
	D =    (int *)     malloc(sizeof(double) * espaciosMatriz);
	DP2 =  (int *)     malloc(sizeof(double) * espaciosMatriz); // cache d^2
	ABCD = (double *)  malloc(sizeof(double) * espaciosMatriz); // resultado a*b + c*d^2
	R =    (double *)  malloc(sizeof(double) * espaciosMatriz);

	// TODO: INICIAR TIMER

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40
		A[i] = B[i] = C[i] = 1.0;

		ABCD[i] = 0.0;
		R[i] = 0.0;
	}

	double tickComienzo, tickFin, op_MinMaxProm;

	int iPos, jPos;

	// COMIENZA OPERACIONES MEDIDAS

	tickComienzo = dwalltime();

	// sacar max, min y prom, y cachear D^2
	
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		else if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		else if (B[i] < MinB) MinA = B[i];

		DP2[i] = D[i] * D[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	op_MinMaxProm = (MaxA * MaxB - MinA * MinB) / PromA * PromB;


	// COMIENZA MULTIPLICACIÓN

	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j + N;
			for (k = 0; k < N; k += BS)
			{
				blkmul(&A[iPos + k], &B[jPos + k], &C[iPos + k], &DP2[jPos + k], &ABCD[iPos + j], N, BS);
			}
			R[iPos + j] = op_MinMaxProm * ABCD[iPos + j];
		}
	}

	tickFin = dwalltime();

	printf("Tiempo de ejecución: %.5lf \n", tickFin - tickComienzo);

	return 0;
}

// COMIENZA PARA MULTIPLICACIÓN POR BLOQUE


void blkmul(double *ablk, double *bblk, double *cblk, int *dblk, double *abcd_blk, int n, int bs)
{
	int i, j, k, iPos, jPos;

	for (i = 0; i < bs; i++)
	{
		iPos = i * n;
		for (j = 0; j < bs; j++)
		{
			jPos = j * n;
			for  (k = 0; k < bs; k++)
			{
				abcd_blk[iPos + j] += (ablk[iPos + k] * bblk[jPos + k]) + (cblk[iPos + k] * dblk[jPos + k]);
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