#!/bin/bash

# 1. Compilar el programa (incluyendo math.h con -lm)
echo "Compilando integral.c..."
mpicc integral.c -o integral -lm

# 2. Configurar el archivo CSV de salida
CSV_FILE="integral_resultados.csv"
echo "N,procesos,ejecucion,tiempo_segundos,valor_estimado" > $CSV_FILE

# 3. Definir las variables a explorar

# ==== CONFIGURACIÓN PARA SERVIDOR HPC (128 NÚCLEOS) ====
# Llevamos N a niveles extremos para que el tiempo de cómputo domine al overhead de MPI.
# 10 Millones, 1 Mil Millones, 100 Mil Millones.
#N_VALUES=(10000 1000000 10000000 1000000000 100000000000)
# Escabilidad agresiva hasta 128 núcleos.
#PROCESOS=(1 2 4 8 16 32 64 96 128)

# ==== CONFIGURACIÓN LOCAL PARA WSL ====
# Si pruebas localmente en tu PC, comenta las dos líneas de arriba y descomenta estas:
N_VALUES=(10000 1000000 100000000 1000000000)
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
