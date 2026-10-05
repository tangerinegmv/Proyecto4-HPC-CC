#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

// Figura irregular (Ejemplo: Círculo x^2 + y^2 <= 1)
// Devuelve 1 si el punto (x,y) está dentro, 0 si está fuera
int dentro_figura_irregular(double x, double y) {
    if (x * x + y * y <= 1.0) {
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <numero_de_simulaciones>\n", argv[0]);
        return 1;
    }

    // Usamos long long para miles de millones de simulaciones
    long long N = atoll(argv[1]);
    
    // Límites de la caja (bounding box) que encierra la figura
    double x_min = -1.0, x_max = 1.0;
    double y_min = -1.0, y_max = 1.0;
    double area_caja = (x_max - x_min) * (y_max - y_min); // 2 * 2 = 4

    int id, n_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);

    // Semilla distinta por proceso
    srand(time(NULL) + id * 1999);

    // Sincronizamos procesos antes de arrancar el cronómetro
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Balance de carga estático
    long long puntos_locales = N / n_procesos;
    long long resto = N % n_procesos;
    if (id < resto) {
        puntos_locales++;
    }

    // Bucle de simulación 2D (Aceptación/Rechazo)
    long long aciertos_locales = 0;
    for (long long i = 0; i < puntos_locales; i++) {
        // Lanzamos un dardo aleatorio dentro de la caja
        double x_rand = x_min + ((double)rand() / RAND_MAX) * (x_max - x_min);
        double y_rand = y_min + ((double)rand() / RAND_MAX) * (y_max - y_min);
        
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
