gcc -o sec secuencial.c -O2
gcc -o pth -pthread pthreads.c -O2
gcc -o omp -fopenmp openmp.c -O2