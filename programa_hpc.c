#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Función 1D de prueba para integración: f(x) = exp(-x^2) en [0, 1]
double funcion_1d(double x) {
    return exp(-x * x);
}

// Función para evaluar área irregular (ejemplo: círculo de radio r=1 dentro de un cuadrado [-1,1]x[-1,1])
int dentro_figura_irregular(double x, double y) {
    // Ecuación del círculo: x^2 + y^2 <= 1
    return (x * x + y * y <= 1.0) ? 1 : 0;
}

int main(int argc, char** argv) {
    int rank, size;
    long long int N_total = 100000000; // 100 millones de simulaciones totales
    long long int N_local;
    
    // Variables para Integral 1D
    double a = 0.0, b = 1.0;
    double suma_local = 0.0, suma_global = 0.0;
    double x, y_rand;

    // Variables para Área Irregular
    long long int aciertos_locales = 0, aciertos_globales = 0;
    double x_min = -1.0, x_max = 1.0;
    double y_min = -1.0, y_max = 1.0;
    double area_rectangulo = (x_max - x_min) * (y_max - y_min);

    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Dividir el trabajo total entre los procesos disponibles
    N_local = N_total / size;

    // Semilla única por proceso para evitar correlaciones en los números aleatorios
    srand(time(NULL) + rank * 1337);

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // 1. Cómputo local: Integración 1D y Muestreo de Área Irregular
    for (long long int i = 0; i < N_local; i++) {
        // --- Cálculo 1D ---
        double rx = (double)rand() / RAND_MAX;
        x = a + (b - a) * rx;
        suma_local += funcion_1d(x);

        // --- Cálculo de Área Irregular ---
        double rx_area = (double)rand() / RAND_MAX;
        double ry_area = (double)rand() / RAND_MAX;
        double px = x_min + (x_max - x_min) * rx_area;
        double py = y_min + (y_max - y_min) * ry_area;
        
        if (dentro_figura_irregular(px, py)) {
            aciertos_locales++;
        }
    }

    // Reducción de resultados usando MPI
    MPI_Reduce(&suma_local, &suma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&aciertos_locales, &aciertos_globales, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    end_time = MPI_Wtime();

    if (rank == 0) {
        // Resultados finales de Integración 1D
        double resultado_integral = (b - a) * (suma_global / N_total);
        
        // Resultados finales de Área Irregular
        double area_estimada = area_rectangulo * ((double)aciertos_globales / N_total);

        printf("==================================================\n");
        printf("RESULTADOS FINALES - PROYECTO 4 (HPC)\n");
        printf("==================================================\n");
        printf("Procesos MPI utilizados: %d\n", size);
        printf("Simulaciones Totales (N): %lld\n", N_total);
        printf("--------------------------------------------------\n");
        printf("Integral Estimada (exp(-x^2)): %.6f\n", resultado_integral);
        printf("Área Irregular Estimada (Círculo): %.6f (Teórico: ~3.14159)\n", area_estimada);
        printf("Tiempo de Ejecución Paralela: %f segundos\n", end_time - start_time);
        printf("==================================================\n");
    }

    MPI_Finalize();
    return 0;
}