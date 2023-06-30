#!/bin/bash
#SBATCH -N 2
#SBATCH --tasks-per-node=4
#SBATCH --exclusive
#SBATCH -o directorioSalida/output_mpi.txt
#SBATCH -e directorioSalida/errores_mpi.txt
mpirun mpi 4096 8
