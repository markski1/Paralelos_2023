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

// Originalmente cacheaba los resultados con el secuencial para luego compararlos mucho mas rapidamente.
// Funcionó bien, pero luego me di cuenta que todo esto se cae a pedazos al usar sbatch en el cluster,
// asi que mala suerte.

/*
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
}*/

// IMPLEMENTACIÓN SECUENCIAL PARA LAS COMPARACIONES
// DEVUELVE R EN Res

void SecuencialEnRDada(int N, int BS, double* A, double *B, double *C, int *D, double *Res) {
	int espaciosMatriz = N * N;
	double *CD, *DP2;

	double PromA, PromB;
	double MaxA = DBL_MIN, MaxB = DBL_MIN;
	double MinA = DBL_MIN, MinB = DBL_MAX;

	double TotalA = 0.0, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes

	DP2 = (double *) malloc(sizeof(double) *       41      ); // cache d^2 ; + 1 espacio para evitar overflow

	// asignaciones
	for (i = 0; i < espaciosMatriz; ++i) {
		Res[i] = 0.0;
	}

	// Cachear un arreglo con pow2 D
	// pueden ser de 1 a 40, y C es zero-indexed, asi que...
	for (i = 0; i < 41; ++i) {
		DP2[i] = (double) (i * i);
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
				blkmm_parte1(&A[iPos + k], &B[jPos + k], &Res[iPos + j], N, BS);
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
				blkmm_parte2(&C[iPos + k], &D[jPos + k], &Res[iPos + j], DP2, N, BS);
			}
		}
	}
}