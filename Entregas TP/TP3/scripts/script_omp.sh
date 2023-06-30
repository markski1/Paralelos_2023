#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --tasks-per-node=1
#SBATCH -o directorioSalida/output_omp.txt
#SBATCH -e directorioSalida/errores_omp.txt
export OMP_NUM_THREADS=8
mpirun --bind-to none omp-mpi 4096 8
