#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

// ==========================================
// SELECCIÓN DE FIGURA IRREGULAR
// Descomenta solo un bloque a la vez
// ==========================================

// --- OPCIÓN 1: CÍRCULO (Radio 1) ---
int dentro_figura_irregular(double x, double y) {
    return (x * x + y * y <= 1.0) ? 1 : 0;
}
void obtener_limites_caja(double *xmin, double *xmax, double *ymin, double *ymax) {
    *xmin = -1.0; *xmax = 1.0;
    *ymin = -1.0; *ymax = 1.0;
}

// --- OPCIÓN 2: ÁREA BAJO LA CURVA X^2 ---
/*
int dentro_figura_irregular(double x, double y) {
    // Solo contamos los dardos que caen por debajo de la curva parabólica
    return (y <= x * x) ? 1 : 0;
}
void obtener_limites_caja(double *xmin, double *xmax, double *ymin, double *ymax) {
    *xmin = 0.0; *xmax = 1.0;
    *ymin = 0.0; *ymax = 1.0;
}
*/

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_simulaciones>\n", argv[0]);
        return 1;
    }

    // Usamos long long para miles de millones de simulaciones
    long long N = atoll(argv[1]);
    
    // Configuración automática de la caja geométrica acoplada a la figura
    double x_min, x_max, y_min, y_max;
    obtener_limites_caja(&x_min, &x_max, &y_min, &y_max);
    double area_caja = (x_max - x_min) * (y_max - y_min);

    int id, n_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);

    // Semilla REPRODUCIBLE. Eliminamos time(NULL).
    // Usamos una base fija + el id multiplicado por un primo grande.
    // Cambiamos a srand48() para tener un PRNG de 48-bits (mayor período).
    long int semilla_base = 12345;
    srand48(semilla_base + id * 999983);

    // Sincronizamos procesos antes de arrancar el cronómetro
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Balance de carga estático perfecto
    long long puntos_locales = N / n_procesos;
    long long resto = N % n_procesos;
    if (id < resto) {
        puntos_locales++;
    }

    // Bucle de simulación 2D (Aceptación/Rechazo)
    long long aciertos_locales = 0;
    for (long long i = 0; i < puntos_locales; i++) {
        // drand48() devuelve automáticamente un double entre 0.0 y 1.0
        double x_rand = x_min + drand48() * (x_max - x_min);
        double y_rand = y_min + drand48() * (y_max - y_min);
        
        // Verificamos si cayó dentro de nuestra figura irregular
        if (dentro_figura_irregular(x_rand, y_rand)) {
            aciertos_locales++;
        }
    }

    // Reducir (sumar) los aciertos de todos los procesos hacia el maestro (0)
    long long aciertos_totales = 0;
    MPI_Reduce(&aciertos_locales, &aciertos_totales, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Medir tiempo final
    double tiempo_fin = MPI_Wtime();

    // El proceso maestro calcula el área y muestra los resultados
    if (id == 0) {
        double area_estimada = area_caja * ((double)aciertos_totales / N);
        
        printf("========================================\n");
        printf("   ÁREA IRREGULAR 2D (Aceptación/Rechazo)\n");
        printf("========================================\n");
        printf("Simulaciones (N): %lld\n", N);
        printf("Procesos MPI    : %d\n", n_procesos);
        printf("Aciertos Totales: %lld\n", aciertos_totales);
        printf("Área estimada   : %.6f\n", area_estimada);
        printf("Tiempo total    : %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("========================================\n");
    }

    MPI_Finalize();
    return 0;
}
