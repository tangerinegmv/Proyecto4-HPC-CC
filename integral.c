#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>

// ==========================================
// SELECCIÓN DE FUNCIÓN PARA INTEGRAR
// Descomenta solo un bloque a la vez
// ==========================================

// --- OPCIÓN 1: CÍRCULO (Radio 1) ---
// La ecuación del círculo es x^2 + y^2 = 1. Despejando 'y', la curva superior es y = sqrt(1 - x^2).
// Multiplicamos por 2 para abarcar tanto el área superior como la inferior del círculo.
double funcion(double x) {
    return 2.0 * sqrt(1.0 - x * x);
}
void obtener_limites_integracion(double *a, double *b) {
    *a = -1.0;
    *b = 1.0;
}

// --- OPCIÓN 2: FUNCIÓN X^2 ---
/*
double funcion(double x) {
    return x * x;
}
void obtener_limites_integracion(double *a, double *b) {
    *a = 0.0;
    *b = 1.0;
}
*/

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_simulaciones>\n", argv[0]);
        return 1;
    }

    // Usamos long long para miles de millones de simulaciones
    long long N = atoll(argv[1]);

    // Desacoplamos los límites matemáticos del bloque principal
    double a, b;
    obtener_limites_integracion(&a, &b);

    int id, n_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);

    // Semilla REPRODUCIBLE. Usamos una base fija + id multiplicado por un primo grande.
    // Usamos srand48 en lugar de srand para obtener un mayor período pseudoaleatorio.
    long int semilla_base = 12345;
    srand48(semilla_base + id * 999983);

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
        // drand48() devuelve automáticamente un double entre 0.0 y 1.0
        double x_rand = a + drand48() * (b - a);
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
        printf("Tiempo total    : %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("========================================\n");
    }

    MPI_Finalize();
    return 0;
}
