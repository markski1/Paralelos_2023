#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stdbool.h>
#include <mpi.h>

#include "funcs_base.h"

// rank del master
#define MASTER       0

// cuantas veces se toman tiempos de runtime y commtime
#define TIME_COUNT   6

// un poco confuso, pero como cada toma de tiempo va sobre el mismo array,
// y cambia de comm a time, esto hace mas facil leer que tiempo se esta tomando.
#define TIME_START   0
#define COMMTIME_1   1
#define COMMTIME_2   2
#define RUNTIME_1    3
#define RUNTIME_2    4
#define TIME_END     5

#define MASTER_PRINT(...) if (rank == MASTER) printf(__VA_ARGS__);

int main(int argc, char * argv[])
{
	// Setup MPI
	int procs, rank;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// validar entrada

	if (argc > 4 || argc < 2 || atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0)
	{
		MASTER_PRINT("Proveer N y blocksize en args. Opcionalmente un '1' para skippear verificacion.\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int BS = atoi(argv[2]);
	int espaciosMatriz = N * N;

	bool comparar = false;

	if (argc == 4 && atoi(argv[3]) == 1) comparar = true;

	if (N % BS != 0)
	{
		MASTER_PRINT("N debe ser multiplo de bs.\n");
		return 1;
	}

	MASTER_PRINT(COLOR_BLUE "Inicializando para:"COLOR_RESET" N=%i ; BS=%i\n", N, BS);

	// declararaciones
	double *A, *B, *C, *R, *CD, *DP2;
	int    *D, *DT2;

	// para el escalar
	double PromA, PromB;
	double MaxA = DBL_MIN, MaxB = DBL_MIN;
	double MinA = DBL_MIN, MinB = DBL_MAX;

	// para hacer reduccion de mpi
	double maxALocal, minALocal, totalALocal;

	double TotalA = 0.0, TotalB = 0.0; // para sacar promedio

	int    i, j, k; // para iteraciónes

	// valores para dividir responsabilidad.

	int responsabilidad = N / procs;
	int responsabilidadPlana = espaciosMatriz / procs;

	int totalDouble = sizeof(double) * espaciosMatriz;
	int divDouble = sizeof(double) * responsabilidadPlana;

	// alocaciones

	if (rank == MASTER)
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
	
	B   = (double *) malloc(totalDouble);
	D   = (int *)    malloc(sizeof(int) * espaciosMatriz);
	DT2 = (int *)    malloc(sizeof(int) * 41); // cache con los posibles valores de D²
	DP2 = (double *) malloc(totalDouble); // cache d^2 en double para usar.

	// asignaciones
	if (rank == MASTER)
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
		for (i = 0; i < responsabilidadPlana; ++i)
		{
			R[i] = 0.0;
		}
	}

	// cachear valores entre 0 y 40 de ^2
	for (int i = 0; i < 41; i++)
	{
		DT2[i] = i * i;
	}

	double escalar,
	       Wtimes[TIME_COUNT], maxWtimes[TIME_COUNT], minWtimes[TIME_COUNT];

	int iPos, jPos;

	MASTER_PRINT("Listo.\n" COLOR_BLUE "Comienza operación..." COLOR_RESET);

	// SYNC E INICIO
	MPI_Barrier(MPI_COMM_WORLD);


	// se comienza a tomar tiempo
	Wtimes[TIME_START] = MPI_Wtime();

	// DISTRIBUIR DATOS

	// con scatter, root (en este caso MASTER) por defecto divide y distribuye.

	MPI_Scatter(
		A,                      // puntero a sendbuf
		responsabilidadPlana,   // # elementos a enviar
		MPI_DOUBLE,             // tipo
		A,                      // puntero a recvbuf
		responsabilidadPlana,   // elementos a recibir
		MPI_DOUBLE,             // tipo a recibir
		MASTER,
		MPI_COMM_WORLD);

	MPI_Scatter(
		C,
		responsabilidadPlana,
		MPI_DOUBLE,
		C,
		responsabilidadPlana,
		MPI_DOUBLE,
		MASTER,
		MPI_COMM_WORLD);

	// con bcast se envia todo de una

	MPI_Bcast(
		B,               // puntero al buffer
		espaciosMatriz,  // # elementos a enviar
		MPI_DOUBLE,      // tipo
		MASTER,
		MPI_COMM_WORLD);

	MPI_Bcast(
		D,
		espaciosMatriz,
		MPI_INT,
		MASTER,
		MPI_COMM_WORLD);

	// se marca tiempo de comunicación 1
	Wtimes[COMMTIME_1] = MPI_Wtime();


	// COMIENZA EJECUCIÓN

	// Cachear DP2

	// fue mediblemente mas rapido hacer eso en cada proc que hacer broadcast
	for (i = 0; i < espaciosMatriz; i++)
	{
		DP2[i] = DT2[D[i]]; // donde DT2 era el cache de resultados indexado por i
	}

	// sacar max, min y prom
	// se toman todos los elementos por igual, asi que no importa seguir los ordenes

	// B se pasa entero, asi que cada cual lo hace localmente.
	for (i = 0; i < espaciosMatriz; ++i)
	{
		TotalB += B[i];
		if (B[i] > MaxB) MaxB = B[i];
		if (B[i] < MinB) MinB = B[i];
	}

	// A se pasa en chunks, asi que cada cual hace su responsabilidad plana
	for (i = 0; i < responsabilidadPlana; ++i)
	{
		TotalA += A[i];
		if (A[i] > MaxA) MaxA = A[i];
		if (A[i] < MinA) MinA = A[i];
	}

	minALocal = MinA;
	maxALocal = MaxA;
	totalALocal = TotalA;

	// se marca tiempo de ejecución 1
	Wtimes[RUNTIME_1] = MPI_Wtime();

	// REDUCIR PROMEDIOS MAXIMOS Y MINIMOS

	MPI_Allreduce(
		&minALocal,      // buffer que envia cada nodo
		&MinA,           // buffer que recibe
		1,               // # de elementos
		MPI_DOUBLE,      // tipo
		MPI_MIN,         // operacion
		MPI_COMM_WORLD);

	MPI_Allreduce(
		&maxALocal,
		&MaxA,
		1,
		MPI_DOUBLE,
		MPI_MAX,
		MPI_COMM_WORLD);

	MPI_Allreduce(
		&totalALocal,
		&TotalA,
		1,
		MPI_DOUBLE,
		MPI_SUM,
		MPI_COMM_WORLD);

	// se marca tiempo de comunicación 2
	Wtimes[COMMTIME_2] = MPI_Wtime();

	// CONTINUA EJECUCIÓN

	PromA = TotalA / espaciosMatriz;
	PromB = TotalB / espaciosMatriz;

	// Aca quedara cacheado el escalar
	escalar = (MaxA * MaxB - MinA * MinB) / (PromA * PromB);

	// COMIENZA MULTIPLICACIÓN

	// Paso 1: Multiplicar A * B, guardar en R.
	for (i = 0; i < responsabilidad; i += BS)
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
	for (i = 0; i < responsabilidadPlana; ++i)
	{
		R[i] = R[i] * escalar;
	}

	// Paso 3: Multiplicar C * Pot2(D); sumar a R
	for (i = 0; i < responsabilidad; i += BS)
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

	// se marca tiempo de ejecución 2
	Wtimes[RUNTIME_2] = MPI_Wtime();

	// FIN DE CALCULOS
	// ahora juntar los resultados en R de MASTER

	MPI_Gather(
		R,                      // puntero a sendbuf de cada nodo
		responsabilidadPlana,   // # de elementos a enviar
		MPI_DOUBLE,             // tipo enviado
		R,                      // puntero a recvbuf de root
		responsabilidadPlana,   // # de elementos a recibir
		MPI_DOUBLE,             // tipo
		MASTER,
		MPI_COMM_WORLD);

	// finaliza tiempo
	Wtimes[TIME_END] = MPI_Wtime();

	// juntamos los tiempos de cada proceso

	MPI_Reduce(
		Wtimes,          // sendbuf de cada proc
		minWtimes,       // a donde recibir el min
		TIME_COUNT,      // # de elementos
		MPI_DOUBLE,      // tipo
		MPI_MIN,         // operación
		MASTER,
		MPI_COMM_WORLD);

	MPI_Reduce(
		Wtimes,
		maxWtimes,
		TIME_COUNT,
		MPI_DOUBLE,
		MPI_MAX,
		MASTER,
		MPI_COMM_WORLD);

	// ya no hace falta nada mas que master
	MPI_Finalize();
	
	if (rank == MASTER)
	{
		double total = Wtimes[TIME_END] - Wtimes[TIME_START];

		double comm;

		//  primer comunicación (scatter y bcast)
		comm = maxWtimes[COMMTIME_1] - minWtimes[TIME_START];

		// segunda comm (Allreduce de mix/max/prom)
		comm += maxWtimes[COMMTIME_2] - minWtimes[RUNTIME_1];

		// tercera comm (gather en R)
		comm += maxWtimes[TIME_END] - minWtimes[RUNTIME_2];

		printf("\n==========\n"COLOR_BLUE"Finaliza operación."COLOR_RESET"\n| Tiempo total : %.5lf\n| Comunicacion : %.5lf\n| Ejecución    : %.5lf\n", total, comm, total - comm);

		if (comparar == false)
		{
			printf("==========\nNo se realizara comparación con versión secuencial. Agregar argumento '1' a lo ultimo para comparar.\n");
			return 0;
		}
		else
		{
			printf("==========\nComparando con version secuencial.\n");
		}

		// y generar un R en R2 con el secuencial
		// se usará para comparar.
		
		double *R2 = (double *) malloc(sizeof(double) * espaciosMatriz);

		SecuencialEnRDada(N, BS, A, B, C, DP2, R2);

		bool error = false;

		for (i = 0; i < espaciosMatriz; ++i)
		{
			if (R[i] != R2[i])
			{
				printf("==========\nERROR EN POSICION %i: R mpi: %lf ; R secuencial: %lf \n", i, R[i], R2[i]);
				error = true;
				break;
			}
		}

		if (!error)
		{
			printf("==========\n" COLOR_VERDE "Exito, los valores son iguales a los del secuencial.\n" COLOR_RESET);
		}
		else
		{
			printf("==========\n" COLOR_ROJO "Error, los valores no son iguales a los del secuencial.\n" COLOR_RESET);
		}

		return 0;
	}
}