#!/bin/bash

ARCHIVO_C="programa_hpc_x2.c"
EJECUTABLE="programa_hpc_x2"
ARCHIVO_CSV="programa_hpc_x2_resultados.csv"
REPETICIONES=10
MAX_PROCESOS=6

# Definimos los diferentes tamaños de simulación para evaluar la convergencia
VALORES_N=(10000 1000000 100000000 1000000000)

mpicc $ARCHIVO_C -o $EJECUTABLE

echo "N_Simulaciones,Procesos,Integral_Estimada,Tiempo" > $ARCHIVO_CSV

for N in "${VALORES_N[@]}"; do
    echo "Analizando convergencia para N = $N..."
    
    for p in $(seq 1 $MAX_PROCESOS); do
        for i in $(seq 1 $REPETICIONES); do
            
            # Pasamos N como argumento al ejecutable
            SALIDA=$(mpirun --oversubscribe -np $p ./$EJECUTABLE $N)
            
            # Capturamos el tiempo y el valor estimado
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo total" | awk '{print $4}')
            INTEGRAL=$(echo "$SALIDA" | grep "Integral estimada" | awk '{print $4}')
            
            echo "$N,$p,$INTEGRAL,$TIEMPO" >> $ARCHIVO_CSV
        done
    done
done

echo "Datos guardados en $ARCHIVO_CSV"