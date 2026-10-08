# Proyecto 4: Integración Numérica y Cálculo de Áreas Irregulares (HPC)

Este repositorio contiene la implementación paralela (MPI) orientada a resolver problemas mediante el Método de Monte Carlo, desarrollada para la asignatura Computación Científica - FACET - UNT.

El objetivo central es comparar el rendimiento, la convergencia y la escalabilidad (Speedup y Eficiencia) de dos enfoques topológicos distintos:
1. **Integración 1D** (Método del Valor Medio).
2. **Cálculo de Área Irregular 2D** (Método de Aceptación/Rechazo).

### 📐 Geometrías Analizadas
Para garantizar una comparación científica y algorítmica impecable, el sistema ha sido diseñado para resolver las mismas figuras bajo los dos enfoques diferentes (1D y 2D):

1. **El Círculo Unitario ($r=1$):**
   * **En Área 2D:** Se lanza dentro de una caja de límites $[-1, 1] \times [-1, 1]$ validando la condición matemática pura $x^2 + y^2 \le 1$.
   * **En Integral 1D:** Se integra analíticamente la mitad superior del círculo mediante la función $f(x) = 2 \sqrt{1 - x^2}$ en el dominio $x \in [-1, 1]$.
   * *Objetivo HPC:* Permite comparar qué operación es más costosa para la CPU: generar dos números aleatorios (x e y) vs generar uno solo pero forzar al hardware a evaluar una raíz cuadrada matemática (`sqrt()`).
2. **La Parábola ($y = x^2$):**
   * **En Área 2D:** Se comprueba si el dardo cae bajo la curva evaluando lógicamente $y \le x^2$ en la caja $[0, 1] \times [0, 1]$.
   * **En Integral 1D:** Se integra clásicamente $f(x) = x^2$ sobre el dominio $[0, 1]$.
   * *Objetivo HPC:* Al ser una función estrictamente multiplicativa y sin carga pesada, expone la latencia cruda de comunicación (overhead de MPI) al escalar a grandes cantidades de nodos.

---

## 🛡️ Decisiones de Diseño

A continuación, se detallan y justifican las decisiones de ingeniería tomadas durante el desarrollo, documentadas específicamente como apoyo para la defensa oral del proyecto.

### 1. Balanceo de Carga (Load Balancing)
* **Lo que hicimos:** Implementamos un **Balanceo de Carga Estático**. Cada proceso recibe una porción matemática idéntica, y el "resto" de la división entera se distribuye equitativamente sumando una iteración extra a los primeros procesos.
* **Lo que NO hicimos:** Decidimos **NO** implementar un esquema de distribución dinámica (Arquitectura Maestro-Trabajador / *Master-Worker*).
* **Justificación:** El Método de Monte Carlo es un problema de naturaleza *Embarrassingly Parallel* (Vergonzosamente Paralelo). El costo de evaluar la función objetivo es idéntico en cada iteración. Un esquema Maestro-Trabajador hubiera introducido una cantidad masiva e innecesaria de comunicación entre los procesos (mensajes MPI de petición de tareas), arruinando la eficiencia.

### 2. Generación de Números Aleatorios (PRNG)
* **Lo que hicimos:** Reemplazamos la función básica `rand()` por la familia `drand48()`. Para la semilla, eliminamos el reloj del sistema (`time(NULL)`) e implementamos una semilla reproducible: `srand48(semilla_base + rank * PRIMO)`.
* **Justificación del Período (`drand48` vs `rand`):** En simulaciones de miles de millones de iteraciones, la función básica de C (`rand()`) se queda corta por su bajo *Período* (la cantidad de números que genera antes de volver a repetir exactamente la misma secuencia circularmente). Al usar `drand48()`, aumentamos el estado matemático a 48-bits, retrasando enormemente el punto en el que los números aleatorios empiezan a ciclar. Esto evita que nuestras simulaciones "revisiten" los mismos puntos una y otra vez, garantizando una convergencia estadística real.
* **Justificación de la Semilla Reproducible y Distanciada:** Al quitar el reloj del sistema, garantizamos que las ejecuciones sean **reproducibles** (vital para poder replicar hallazgos en la investigación). Además, para evitar que los procesos MPI arranquen con secuencias idénticas, sumamos su `rank` multiplicado por un número primo grande (999983). Matemáticamente, esto aleja drásticamente el punto de partida (estado inicial) de cada proceso dentro de la ruleta aleatoria, evitando que sus flujos se superpongan prematuramente.

### 3. Metodología de Medición y Análisis
* **Lo que hicimos:** Automatizar ejecuciones repetidas para el mismo tamaño de problema y recolectar los tiempos, para luego calcular métricas de rendimiento basadas en la **Mediana**.
* **Justificación de Múltiples Ejecuciones:** El tiempo de ejecución en cualquier sistema operativo moderno no es perfectamente determinista. Existen interrupciones de hardware, cambios de contexto y procesos de fondo que pueden introducir demoras aleatorias en una ejecución ("ruido del sistema"). Tomar múltiples muestras nos permite obtener una distribución estadística confiable del comportamiento real de nuestro código.
* **Justificación de usar la Mediana (y no el Promedio):** El promedio (la media matemática) es muy sensible a valores atípicos (*outliers*). Si una ejecución se ralentiza fuertemente porque el servidor decidió ejecutar una tarea de fondo en ese segundo, el promedio se dispararía hacia arriba engañosamente. La mediana, por el contrario, selecciona el valor central de nuestras mediciones, ignorando los extremos anómalos y otorgándonos el "tiempo típico real" del algoritmo en condiciones normales.


## 📊 Análisis de los Resultados

### Expectativas Teóricas
En base a los fundamentos de HPC y de los métodos estocásticos, a la hora de interpretar las gráficas obtenemos los siguientes comportamientos esperados:
- **Convergencia del Error Absoluto:** Por la Ley de los Grandes Números, a medida que $N$ aumenta, el error debe decrecer de forma inversamente proporcional a la raíz cuadrada de $N$ ($1/\sqrt{N}$). En nuestra gráfica Log-Log, esto se debe observar como una línea recta descendente y constante.
- **Speedup Real vs Ideal:** El Speedup Ideal traza una línea recta (donde $P$ procesos reducen el tiempo $P$ veces). En la realidad, la línea de nuestro código correrá por debajo de la ideal. La separación entre ambas líneas representa el *overhead* (el costo de inicializar MPI, balancear variables, y la comunicación final en el `MPI_Reduce`).
- **Eficiencia:** La eficiencia inicia en 1.0 (o 100%) para 1 proceso. Debería ir decayendo levemente a medida que agregamos núcleos (Ley de Amdahl). Esta caída es consecuencia de que el bus de memoria caché se satura cuando todos los procesadores intentan consultar datos o escribir variables simultáneamente.

### Plantilla de Resultados Numéricos
*(Completar esta tabla una vez que se ejecuten los scripts en el servidor adecuado para la defensa)*

| Tamaño Problema (N) | N° Procesos | Tiempo Mediana (s) | Error Absoluto | Speedup Calculado | Eficiencia Calculada |
|---------------------|-------------|--------------------|----------------|-------------------|----------------------|
| 10.000              | 1           | [Completar]        | [Completar]    | 1.0x              | 1.0                  |
| 10.000              | MAX         | [Completar]        | [Completar]    | [Completar]       | [Completar]          |
| 1.000.000.000       | 1           | [Completar]        | [Completar]    | 1.0x              | 1.0                  |
| 1.000.000.000       | MAX         | [Completar]        | [Completar]    | [Completar]       | [Completar]          |

<!-- 
💡 CONSEJOS Y RECOMENDACIONES OCULTAS PARA TU DEFENSA ORAL: 
1. Anomalías con N pequeños (ej. N=10.000): Si observas que con 10.000 simulaciones usar 8 procesos demora MÁS TIEMPO que usar 1 proceso (Speedup menor a 1), ¡no te asustes y úsalo a tu favor! Explícale al profesor que "para problemas diminutos, el costo de coordinar y enviar mensajes entre los nodos de MPI demora más tiempo que simplemente resolver la matemática localmente".
2. Caídas bruscas de Eficiencia: Si pruebas en un servidor de 16 núcleos y notas que al pasar del núcleo 8 al 16 la eficiencia se desploma, menciona la arquitectura del procesador. Usualmente es porque esos núcleos extra son "lógicos" (Hyper-Threading) y no núcleos físicos independientes, por lo que se pelean por la misma memoria física caché L1/L2.
3. Diferencia Integral vs Área: En la defensa, compara las curvas de las dos implementaciones. Generalmente, el Área 2D requiere generar más números aleatorios (dos por dardo) y una condicional (if x^2+y^2). Fíjate si esa carga de cálculo local extra hace que aproveche mejor el paralelismo (oculta mejor las latencias de MPI) que la Integral 1D.
-->
