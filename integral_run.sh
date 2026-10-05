#!/bin/bash

# 1. Compilar el programa
echo "Compilando integral.c..."
mpicc integral.c -o integral

# 2. Configurar el archivo CSV de salida
CSV_FILE="integral_resultados.csv"
echo "N,procesos,ejecucion,tiempo_segundos,valor_estimado" > $CSV_FILE

# 3. Definir las variables a explorar
N_VALUES=(10000 1000000 100000000 1000000000)

# Escribe manualmente aquí la lista exacta de procesos que quieres probar.
# Si MPI arroja error, usa números menores a la cantidad de tus núcleos físicos reales.
PROCESOS=(1 2 4 6)

# La cátedra pide la "mediana de múltiples ejecuciones". Haremos 5 repeticiones.
EJECUCIONES=5

echo "Iniciando simulaciones masivas para Integral 1D..."
echo "---------------------------------------------------"

# 4. Bucle anidado para correr todas las combinaciones
for N in "${N_VALUES[@]}"; do
    for p in "${PROCESOS[@]}"; do
        for (( e=1; e<=EJECUCIONES; e++ )); do
            echo "Corriendo -> N: $N | Procesos: $p | Ejecución: $e de $EJECUCIONES..."
            
            # Ejecutamos con MPI y guardamos toda la salida de texto en la variable SALIDA
            SALIDA=$(mpirun -np $p ./integral $N)
            
            # Usamos grep y awk para extraer el tiempo y el valor estimado
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo total" | awk -F ":" '{print $2}' | awk '{print $1}')
            ESTIMADO=$(echo "$SALIDA" | grep "est\." | awk -F ":" '{print $2}' | awk '{print $1}')
            
            # Guardamos la fila en nuestro archivo CSV
            echo "$N,$p,$e,$TIEMPO,$ESTIMADO" >> $CSV_FILE
        done
    done
done

echo "---------------------------------------------------"
echo "¡Análisis de la Integral completado!"
echo "Tus resultados puros están guardados en: $CSV_FILE"
