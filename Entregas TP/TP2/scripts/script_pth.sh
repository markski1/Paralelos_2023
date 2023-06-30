#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -o directorioSalida/output_pth.txt
#SBATCH -e directorioSalida/errores_pth.txt
./pth 4096 8 8