#!/bin/bash

# 1. Compilar el programa (incluyendo math.h con -lm)
echo "Compilando integral.c..."
mpicc integral.c -o integral -lm

# 2. Selección de entorno y configuración del archivo CSV
# Puede pasarse como argumento: ./integral_run.sh [pc|servidor] [nombre_figura]
ENTORNO="${1:-}"
FIGURA="${2:-circulo}"

if [ -z "$ENTORNO" ]; then
    echo "=========================================="
    echo "  Selección de Entorno de Ejecución"
    echo "=========================================="
    echo "1) PC Local (AMD Ryzen 6 cores, 8 GB RAM)"
    echo "2) Servidor HPC (96 cores, 128 GB RAM)"
    read -p "Ingresa opción [1 para PC, 2 para Servidor] (por defecto 1): " OPCION
    if [ "$OPCION" == "2" ] || [ "$OPCION" == "servidor" ]; then
        ENTORNO="servidor"
    else
        ENTORNO="pc"
    fi
fi

if [ "$ENTORNO" == "servidor" ] || [ "$ENTORNO" == "server" ]; then
    CSV_FILE="integral_${FIGURA}_servidor.csv"
    # Configuración para Servidor HPC (96 núcleos, 128 GB RAM)
    N_VALUES=(10000 1000000 10000000 1000000000)
    PROCESOS=(1 2 4 8 16 32 64 96)
    echo ">>> Modo seleccionado: SERVIDOR HPC"
    echo ">>> Archivo de salida: $CSV_FILE"
    echo ">>> Procesos: ${PROCESOS[*]}"
else
    CSV_FILE="integral_${FIGURA}_pc.csv"
    # Configuración para PC local (6 núcleos, 8 GB RAM)
    N_VALUES=(10000 1000000 10000000 1000000000)
    PROCESOS=(1 2 4 6)
    echo ">>> Modo seleccionado: PC LOCAL"
    echo ">>> Archivo de salida: $CSV_FILE"
    echo ">>> Procesos: ${PROCESOS[*]}"
fi

echo "N,procesos,ejecucion,tiempo_segundos,valor_estimado" > $CSV_FILE

# Haremos 5 repeticiones.
EJECUCIONES=5

echo "Iniciando simulaciones masivas para Integral 1D..."
echo "---------------------------------------------------"

# 4. Bucle anidado para correr todas las combinaciones
for N in "${N_VALUES[@]}"; do
    for p in "${PROCESOS[@]}"; do
        for (( e=1; e<=EJECUCIONES; e++ )); do
            echo "Corriendo -> N: $N | Procesos: $p | Ejecución: $e de $EJECUCIONES..."
            
            # Usamos --oversubscribe para que Open MPI permita usar todos los hilos lógicos
            SALIDA=$(mpirun --oversubscribe -np $p ./integral $N 2>&1)
            
            # Usamos grep y awk para extraer el tiempo y el valor estimado
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo total" | awk -F ":" '{print $2}' | awk '{print $1}')
            ESTIMADO=$(echo "$SALIDA" | grep "est\." | awk -F ":" '{print $2}' | awk '{print $1}')
            
            if [ -z "$TIEMPO" ] || [ -z "$ESTIMADO" ]; then
                echo "⚠️ ERROR en ejecución (N=$N, p=$p):"
                echo "$SALIDA"
            fi

            # Guardamos la fila en nuestro archivo CSV
            echo "$N,$p,$e,$TIEMPO,$ESTIMADO" >> $CSV_FILE
        done
    done
done

echo "---------------------------------------------------"
echo "¡Análisis de la Integral completado!"
echo "Tus resultados puros están guardados en: $CSV_FILE"
