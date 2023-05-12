#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -o directorioSalida/output.txt
#SBATCH -e directorioSalida/errores.txt
export OMP_NUM_THREADS=$TP2_THREADS
./omp $TP2_N $TP2_BS