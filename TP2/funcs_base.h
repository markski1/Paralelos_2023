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

//                        C           D              CD             Cache Pot2(D)
void blkmm_parte2(double *C_blk, int *D_blk, double *R_blk, double *D_2, int n, int bs)
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
			R_blk[iPos + j] += registro; // SUMA, no igual, porque ya tenemos R con Escalar * [A * B],
		}
	}
}