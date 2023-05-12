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

//                        C           D              R              Cache Pot2(D)
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


// COMIENZA FUNCIONES PARA CHEQUEO

// Usado por el secuencial, escribe la cache
void EscribirCache(double *R, int N) {
	char nombreArchivo[20];

	sprintf(nombreArchivo, "matriz_%i.txt", N);

	FILE *cache = fopen(nombreArchivo, "w");

	int NN = N * N;

	for (int i = 0; i < NN; ++i) {
		fprintf(cache, "%lf\n", R[i]);
	}

	fclose(cache);

	printf("\n===========\nCache escrita. Ahora se comparará el resultado con N=%i en los algoritmos paralelos.\n", N);
}

// Usado por los paralelos, comparan con la cache.
void CompararCache(double *R, int N) {
	char nombreArchivo[20];

	sprintf(nombreArchivo, "matriz_%i.txt", N);

	FILE *cache = fopen(nombreArchivo, "r");

	int cuenta = 0;

	if (cache) {
		char * linea = NULL;
	    size_t len = 0;
	    ssize_t read;

	    double lectura;

	    while ((read = getline(&linea, &len, cache)) != -1) {
	    	sscanf(linea, "%lf", &lectura);
	    	if (lectura == R[cuenta]) {
	    		cuenta++;
	    		continue;
	    	}
	    	else {
	    		printf("==============\nError, los valores no son iguales a los del secuencial.\n");
	    		return;
	    	}
	    }

	    printf("==============\nExito, los valores son iguales a los del secuencial.\n");
	}
	else {
		printf("==============\nNo se pudo comparar, porque aun no se ejecuto el secuencial con este valor de N.\n");
	}
}