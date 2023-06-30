#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -o directorioSalida/output_omp.txt
#SBATCH -e directorioSalida/errores_omp.txt
export OMP_NUM_THREADS=8
./omp 4096 8