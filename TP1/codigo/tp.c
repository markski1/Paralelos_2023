#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double dwalltime();
void matmulblks(double *a, double *b, double *c, int n, int bs);
void blkmul(double *ablk, double *bblk, double *cblk, int n, int bs);

int main(int argc, char * argv[]) {
	if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 ) {
		printf("Proveer N y blocksize en args.");
		return 1;
	}

	int N = atoi(argv[1]);
	int bloque = atoi(argv[2]);
	int espaciosMatriz = N * N;

	if (N % bloque != 0) {
		printf("N debe ser multiplo de bloque.");
		return 1;
	}

	// declararaciones
	double *A, *B, *C, *R;
	int *D;

	double PromA, PromB = 0.0;
	double MaxA, MaxB = DBL_MIN;
	double MinA, MinB = DBL_MAX;

	double TotalA, TotalB = 0.0; // para sacar promedio

	int i, j;

	// alocaciones
	A = (double *) malloc(sizeof(double) * espaciosMatriz);
	B = (double *) malloc(sizeof(double) * espaciosMatriz); // columnas
	C = (double *) malloc(sizeof(double) * espaciosMatriz);
	R = (double *) malloc(sizeof(double) * espaciosMatriz);
	D = (int *)    malloc(sizeof(double) * espaciosMatriz); // columnas

	// TODO: INICIAR TIMER

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		D[i] = (rand() % 40) + 1; // deben ser valores entre 1 y 40
		A[i] = B[i] = C[i] = 1.0;
	}

	double tickComienzo, tickFin, op_MinMaxProm;

	// cachear posiciones
	int iPos, jPos;

	tickComienzo = dwalltime();

	// sacar max, min y prom
	
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

	op_MinMaxProm = (MaxA * MaxB - MinA * MinB) / PromA * PromB;

	for (i = 0; i < N; ++i) {
		iPos = i * N;
		for (j = 0; j < N; ++j) {
			jPos = j * N;
			for (int k = 0; k < N; ++k) {
				R[iPos+j] = op_MinMaxProm * (A[iPos+k] * B[k+jPos]) + (C[iPos+k] * (D[k+jPos] * D[k+jPos]));
			}
		}
	}

	tickFin = dwalltime();

	printf("Tiempo de ejecución: %.10lf \n", tickFin - tickComienzo);

	return 0;
}

// COMIENZA FUNCIONES PARA MULTIPLICACIÓN POR BLOQUE


void matmulblks(double *a, double *b, double *c, int n, int bs)
{
	int i, j, k;
	
	for (i = 0; i < n; i += bs)
	{
		for (j = 0; j < n; j += bs)
		{
			for  (k = 0; k < n; k += bs)
			{
				blkmul(&a[i*n + k], &b[j*n + k], &c[i*n + j], n, bs);
			}
		}
	}
}

void blkmul(double *ablk, double *bblk, double *cblk, int n, int bs)
{
	int i, j, k;

	for (i = 0; i < bs; i++)
	{
		for (j = 0; j < bs; j++)
		{
			for  (k = 0; k < bs; k++)
			{
				cblk[i*n + j] += ablk[i*n + k] * bblk[j*n + k];
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
