#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -o directorioSalida/output_sec.txt
#SBATCH -e directorioSalida/errores_sec.txt
./sec 4096 8