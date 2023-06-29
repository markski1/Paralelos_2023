#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <mpi.h>

#include "funcs_base.h"

#define COORD  0

#define COMMS  6
#define RUNST  6

#define COORD_PRINT(...) if (rank == COORD) printf(__VA_ARGS__);

int main(int argc, char * argv[])
{
	if (argc != 3 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 )
	{
		printf("Proveer N y blocksize en args.\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);
	int espaciosMatriz = N * N;

	if (N % BS != 0)
	{
		printf("N debe ser multiplo de bs.\n");
		return 1;
	}

	printf(COLOR_BLUE "Inicializando para:"COLOR_RESET" N=%i ; BS=%i\n", N, BS);

	// declararaciones
	double *A, *B, *C, *R, *CD, *DP2;
	int    *D, *DT2;

	double PromA, PromB;
	double MaxA = DBL_MIN, MaxB = DBL_MIN;
	double MinA = DBL_MIN, MinB = DBL_MAX;

	double TotalA = 0.0, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes


	// MPI
	int procs, rank;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
          
	// valores para dividir responsabilidad.

	int responsabilidad = N / procs;
	int totalResponsabilidad = responsabilidad * N;

	int totalDouble = sizeof(double) * espaciosMatriz;
	int divDouble = sizeof(double) * totalResponsabilidad;

	if (rank == COORD)
	{
		A   = (double *) malloc(totalDouble);
		C   = (double *) malloc(totalDouble);
		R   = (double *) malloc(totalDouble);
	}
	else
	{
		A   = (double *) malloc(divDouble);
		C   = (double *) malloc(divDouble);
		R   = (double *) malloc(divDouble);
	}

	// alocaciones
	
	B   = (double *) malloc(totalDouble);
	D   = (int *)    malloc(sizeof(int)    * espaciosMatriz);
	DT2 = (int *)    malloc(sizeof(int) * 41); // cache con los posibles valores de D²
	DP2 = (double *) malloc(totalDouble); // cache d^2 en double para usar.

	// asignaciones
	if (rank == COORD)
	{
		for (i = 0; i < espaciosMatriz; ++i)
		{
			D[i] = (rand() % 40) + 1; // valores al azar, entre 1 y 40

			A[i] = B[i] = C[i] = 1.0;
			R[i] = 0.0;
		}
	}
	else
	{
		for (i = 0; i < totalResponsabilidad; ++i)
		{
			R[i] = 0.0;
		}
	}

	for (int i = 0; i < 41; i++)
	{
		DT2[i] = i * i;
	}

	double runsTiming[RUNST], escalar,
	       commsTiming[COMMS], maxCommTiming[COMMS], minCommTiming[COMMS];

	int iPos, jPos;

	COORD_PRINT("Listo.\n" COLOR_BLUE "Comienza operación..." COLOR_RESET);

	// SYNC E INICIO
	MPI_Barrier(MPI_COMM_WORLD);

	commsTiming[0] = MPI_Wtime();

	// DISTRIBUIR DATOS

	// con scatter, root (en este caso COORD) por defecto divide y distribuye.

	// pongo los argumentos en fila y comentados para propia referenci

	MPI_Scatter(
		A,                      // puntero a sendbuf
		totalResponsabilidad,   // # elementos a enviar
		MPI_DOUBLE,             // tipo
		A,                      // puntero a recvbuf
		totalResponsabilidad,   // elementos a recibir
		MPI_DOUBLE,             // tipo a recibir
		COORD,
		MPI_COMM_WORLD);

	MPI_Scatter(
		C,
		totalResponsabilidad,
		MPI_DOUBLE,
		C,
		totalResponsabilidad,
		MPI_DOUBLE,
		COORD,
		MPI_COMM_WORLD);

	// con bcast se envia todo de una

	// de nuevo, pongo los argumentos en fila y comentados para propia referencia.

	MPI_Bcast(
		B,               // puntero al buffer
		espaciosMatriz,  // # elementos a enviar
		MPI_DOUBLE,      // tipo
		COORD,
		MPI_COMM_WORLD);

	MPI_Bcast(
		D,
		espaciosMatriz,
		MPI_INT,
		COORD,
		MPI_COMM_WORLD);

	// COMIENZA EJECUCIÓN

	runsTiming[0] = dwalltime();

	// Cachear DP2

	for (i = 0; i < espaciosMatriz; i++)
	{
		DP2[i] = DT2[D[i]]; // donde DT2 era el cache de resultados indexado por i
	}

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes
	for (i = 0; i < responsabilidad; ++i)
	{
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

	// Paso 1: Multiplicar A * B, guardar en R.
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&A[iPos + k], &B[jPos + k], &R[iPos + j], N, BS);
			}
		}
	}

	// Paso 2: Multiplica R por el escalar.
	for (i = 0; i < espaciosMatriz; ++i)
	{
		R[i] = R[i] * escalar;
	}

	// Paso 3: Multiplicar C * Pot2(D); sumar a R
	for (i = 0; i < N; i += BS)
	{
		iPos = i * N;
		for (j = 0; j < N; j += BS)
		{
			jPos = j * N;
			for (k = 0; k < N; k += BS)
			{
				blkmm(&C[iPos + k], &DP2[jPos + k], &R[iPos + j], N, BS);
			}
		}
	}
	

	// FIN CUENTA
	runsTiming[RUNST] = dwalltime();

	double runTotal = 0.0;

	COORD_PRINT("\n==============\n"COLOR_BLUE"Finaliza operación."COLOR_RESET" Tiempo: %.5lf\n", runTotal);

	return 0;
}