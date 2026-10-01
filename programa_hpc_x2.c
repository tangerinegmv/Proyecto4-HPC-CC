#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

// Funcion a integrar: f(x) = x^2
double funcion(double x) {
    return x * x;
}

int main(int argc, char** argv) {
    // Usamos long long para permitir miles de millones de simulaciones sin desbordamiento
    long long N = atoll(argv[1]);
    double a = 0.0;
    double b = 1.0;

    int id, n_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);

    // CRITICO: Cada proceso necesita una semilla distinta para no repetir los mismos numeros
    srand(time(NULL) + id);

    double tiempo_inicio;
    if (id == 0) tiempo_inicio = MPI_Wtime();

    // Balance de carga estatico: dividimos la cantidad total de puntos a generar
    long long puntos_locales = N / n_procesos;
    long long resto = N % n_procesos;
    if (id < resto) {
        puntos_locales++;
    }

    // Bucle de simulacion (Sin arreglos para no saturar la RAM)
    double suma_local = 0.0;
    for (long long i = 0; i < puntos_locales; i++) {
        // Generar un número aleatorio real en el intervalo [a, b]
        double x_rand = a + ((double)rand() / RAND_MAX) * (b - a);
        
        // Evaluar la función en ese punto aleatorio y acumular
        suma_local += funcion(x_rand);
    }

    // Reducir la suma de todos los procesos al proceso maestro
    double suma_total = 0.0;
    MPI_Reduce(&suma_local, &suma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // El proceso maestro calcula el promedio y el área final
    if (id == 0) {
        // Fórmula de Monte Carlo: (b - a) * (promedio de las evaluaciones)
        double integral = (b - a) * (suma_total / N);
        double tiempo_fin = MPI_Wtime();

        printf("Simulaciones (N): %lld\n", N);
        printf("Integral estimada : %f\n", integral);
        printf("Valor analítico   : 0.333333...\n");
        printf("Tiempo total      : %.6f segundos\n", tiempo_fin - tiempo_inicio);
    }

    MPI_Finalize();
    return 0;
}