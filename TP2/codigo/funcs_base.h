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

// COMIENZA FUNCIONES PARA MULTIPLICACIÓN POR BLOQUE

//                        A              B              R
void blkmm(double *A_blk, double *B_blk, double *R_blk, int n, int bs)
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
			R_blk[iPos + j] += registro;
		}
	}
}

// IMPLEMENTACIÓN SECUENCIAL PARA LAS COMPARACIONES
// DEVUELVE R EN Res

void SecuencialEnRDada(int N, int BS, double* A, double *B, double *C, double *DP2, double *Res) {
	int espaciosMatriz = N * N;

	double PromA, PromB;
	double MaxA = DBL_MIN, MaxB = DBL_MIN;
	double MinA = DBL_MIN, MinB = DBL_MAX;

	double TotalA = 0.0, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		Res[i] = 0.0;
	}

	double escalar;

	int iPos, jPos;

	// COMIENZA OPERACIONES MEDIDAS

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = 0; i < espaciosMatriz; ++i) {
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		if (A[i] < MinA) MinA = A[i];

		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		if (B[i] < MinB) MinB = B[i];
	}

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	// COMIENZA MULTIPLICACIÓN

	// Paso 1: Multiplicar A * B, guardar en Res.
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&A[iPos + k], &B[jPos + k], &Res[iPos + j], N, BS);
			}
		}
	}

	// Paso 2: Multiplica Res por el escalar.
	for (i = 0; i < espaciosMatriz; ++i) {
		Res[i] = Res[i] * escalar;
	}

	// Paso 3: Multiplicar C * Pot2(D); sumar a Res
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&C[iPos + k], &DP2[jPos + k], &Res[iPos + j], N, BS);
			}
		}
	}
}