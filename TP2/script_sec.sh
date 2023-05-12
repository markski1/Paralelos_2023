#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -o directorioSalida/output_sec.txt
#SBATCH -e directorioSalida/errores_sec.txt
./sec 2048 8