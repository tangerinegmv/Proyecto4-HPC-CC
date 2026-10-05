#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

// Función a integrar en 1D: f(x) = x^2
double funcion(double x) {
    return x * x;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_simulaciones>\n", argv[0]);
        return 1;
    }

    // Usamos long long para permitir miles de millones de simulaciones sin desbordamiento
    long long N = atoll(argv[1]);
    double a = 0.0;
    double b = 1.0;

    int id, n_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);

    // Semilla distinta por proceso para la aleatoriedad
    srand(time(NULL) + id * 1999);

    // Sincronizamos todos los procesos antes de iniciar el reloj
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Balance de carga estático perfecto (repartiendo el resto)
    long long puntos_locales = N / n_procesos;
    long long resto = N % n_procesos;
    if (id < resto) {
        puntos_locales++;
    }

    // Bucle de simulación 1D
    double suma_local = 0.0;
    for (long long i = 0; i < puntos_locales; i++) {
        double x_rand = a + ((double)rand() / RAND_MAX) * (b - a);
        suma_local += funcion(x_rand);
    }

    // Reducir la suma de todos los procesos al proceso maestro (0)
    double suma_total = 0.0;
    MPI_Reduce(&suma_local, &suma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Medir tiempo final
    double tiempo_fin = MPI_Wtime();

    // El proceso maestro calcula el promedio, el área final y muestra resultados
    if (id == 0) {
        double integral = (b - a) * (suma_total / N);
        
        printf("========================================\n");
        printf("   INTEGRAL 1D (Método Valor Medio)\n");
        printf("========================================\n");
        printf("Simulaciones (N): %lld\n", N);
        printf("Procesos MPI    : %d\n", n_procesos);
        printf("Integral est.   : %f\n", integral);
        printf("Valor analítico : 0.333333...\n");
        printf("Tiempo total    : %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("========================================\n");
    }

    MPI_Finalize();
    return 0;
}
