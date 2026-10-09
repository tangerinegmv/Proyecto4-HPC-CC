# Proyecto 4: Integración Numérica y Cálculo de Áreas Irregulares (HPC)

Este repositorio contiene la implementación paralela (MPI) orientada a resolver problemas mediante el Método de Monte Carlo, desarrollada para la asignatura Computación Científica - FACET - UNT.

El objetivo central es comparar el rendimiento, la convergencia y la escalabilidad (Speedup y Eficiencia) de dos enfoques topológicos distintos:
1. **Integración 1D** (Método del Valor Medio).
2. **Cálculo de Área Irregular 2D** (Método de Aceptación/Rechazo).

### Geometrías Analizadas
Hemos decidido abordar el proyecto analizando dos funciones distintas, las cuales se someteran a ambos algoritmos, ésto con el fin de tener un estudio más completo, para poder comparar resultados y llegar a una conclusión más robusta, siguiendo con lo pedido en la consigna. Las figuras elegidas son:

1. **El Círculo Unitario ($r=1$):**
   * **En Integral 1D:** Se integra analíticamente la mitad superior del círculo mediante la función $f(x) = 2 \sqrt{1 - x^2}$ en el dominio $x \in [-1, 1]$.
   * **En Área 2D:** Se lanza dentro de una caja de límites $[-1, 1] \times [-1, 1]$ validando la condición matemática $x^2 + y^2 \le 1$.
   * *Objetivo HPC:* Permite comparar qué operación es más costosa para la CPU: generar dos números aleatorios (x e y) vs generar uno solo pero forzar al hardware a evaluar una raíz cuadrada matemática (`sqrt()`).
2. **La Parábola ($y = x^2$):**
   * **En Integral 1D:** Se integra clásicamente $f(x) = x^2$ sobre el dominio $[0, 1]$.
   * **En Área 2D:** Se comprueba si el dardo cae bajo la curva evaluando lógicamente $y \le x^2$ en la caja $[0, 1] \times [0, 1]$.
   * *Objetivo HPC:* Al ser una función estrictamente multiplicativa y sin carga pesada, expone la latencia cruda de comunicación (overhead de MPI) al escalar a grandes cantidades de nodos.

---

## Decisiones de Diseño

A continuación, se detallan y justifican las decisiones de ingeniería tomadas durante el desarrollo.

### 1. Balanceo de Carga (Load Balancing)
* Implementamos un **Balanceo de Carga Estático**. Cada proceso recibe una porción matemática idéntica, y el "resto" de la división entera se distribuye equitativamente sumando una iteración extra a los primeros procesos. Decidimos esto pues el método de Monte Carlo es un problema de naturaleza *Embarrassingly Parallel* (Vergonzosamente Paralelo), el costo de evaluar la función objetivo es idéntico en cada iteración y, al no existir intercambio de mensajes ni consultas en tiempo real entre los hilos o procesos para pedir más tareas, los procesadores aprovechan el 100% de su ciclo de reloj en calcular. Un esquema Maestro-Trabajador hubiera introducido una cantidad masiva e innecesaria de comunicación entre los procesos (mensajes MPI de petición de tareas), arruinando la eficiencia.


### 2. Elección del Generador Pseudoaleatorio (PRNG) y Reproducibilidad:
* Inicialmente consideramos la implementación de la función clásica rand() de la biblioteca estandar de C que vimos en clase. Sin embargo, identificamos que, a gran escala (miles de millones), el generador agota su ciclo y comienza a repetir la misma secuencia de numeros. Esto sucede porque RAND_MAX también está en las cifras de los miles de millones (10^9 aprox).
 * Por esta razon, optamos por usar la familia drand48, estándar en POSIX, ya que, amplía el espacio de estados a 48 bits (10^14 aprox.) lo cual garantiza que el ciclo no se agote en corridas masivas, y genera directamente un numero punto flotante double, ahorrando costo de conversión y división en cada iteración. Además, al ser estándar POSIX de C (stdlib.h), compila directamente con mpicc en cualquier PC o clúster sin necesidad de compilar paquetes de terceros.
 * También nos planteamos la idea de la reproducibilidad de los resultados, por lo que reemplazamos el uso de time(NULL), que dependía del uso del reloj del sistema, y, en su lugar, implementamos un esquema de semillas reproducibles distanciadas, para poder comparar el resultado de los experimentos.

### 3. Metodología de Medición y Análisis
* Lo que hicimos fue implementar el Método de Monte Carlo de Integración y Cálculo de área en dos funciones/figuras distintas, haciendo uso de la paralelización con MPI, para analizar su eficiencia. 
* Utilizamos la función **MPI_Reduce** que realiza una operación de reducción global (como puede ser calcular el máximo, la suma, hacer un AND lógico, etc) sobre cada uno de los miembros del grupo. La operación de reducción puede ser tanto una predefinida de la lista de operaciones como una operación definida por el usuario. En nuestro caso la utilizamos para el cálculo de la función en N puntos entre a y b para la integral, y para la generación de los N puntos de pruba para el método de Aceptación/Rechazo.
* Se utilizó un único MPI_Barrier previo a la captura del tiempo inicial con MPI_Wtime(). Si bien, no es necesario para nuestro programa, ya que la operación de reducción es bloqueante de por sí, se tomó esta desición con el fin de aportar buenas practicas y rigurosidad al trabajo, ya que, la operación, garantiza la consistencia del benchmark al sincronizar la largada de todos los procesos distribuidos, evitando sesgos por el retardo de inicialización del runtime de MPI. No se incluyeron barreras adicionales dentro del bucle de simulación para preservar el asincronismo y evitar el degradamiento por el Efecto del Eslabón Más Lento (Straggler Effect). 
* **Medición del tiempo**: A la hora de tomar los tiempos de ejecución de la paralelización, surgió el planteo de cuándo deberímos tomar el tiempo de fin (el tiempo de inicio era claro). La idea original fue la de hacerlo justo después de la operación de reducción,es decir, medir estrictamente el algoritmo paralelo. Pero, se nos ocurrió que, el cálculo que se hace dentro del proceso raíz, podría afectar al tiempo total y ser significativo. Luego de investigar en foros, decidimos optar por la primera alternativa, ya que es la regla metodológica obligatoria en la comunidad de HPC, y el cálculo es trabajo serial secundario del proceso 0 y no significa un costo significativo de tiempo.



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

### Bibliografía
- [Dynamic Load Balancing of Parallel Monte Carlo Transport Calculations](https://www.osti.gov/biblio/15015938)
- [drand48](https://pubs.opengroup.org/onlinepubs/007904975/functions/drand48.html)
- [MPI_Reduce](https://lsi2.ugr.es/jmantas/ppr/ayuda/mpi_ayuda.php?ayuda=MPI_Reduce)
- [When do I need to use MPI_Barrier()?](https://stackoverflow.com/questions/13305814/when-do-i-need-to-use-mpi-barrier)
- [MPI global execution time](https://stackoverflow.com/questions/5298739/mpi-global-execution-time)
- [How do I interpret the results from MPI_Wtime()?](https://scicomp.stackexchange.com/questions/29876/how-do-i-interpret-the-results-from-mpi-wtime)

<!-- 
💡 CONSEJOS Y RECOMENDACIONES OCULTAS PARA TU DEFENSA ORAL: 
1. Anomalías con N pequeños (ej. N=10.000): Si observas que con 10.000 simulaciones usar 8 procesos demora MÁS TIEMPO que usar 1 proceso (Speedup menor a 1), ¡no te asustes y úsalo a tu favor! Explícale al profesor que "para problemas diminutos, el costo de coordinar y enviar mensajes entre los nodos de MPI demora más tiempo que simplemente resolver la matemática localmente".
2. Caídas bruscas de Eficiencia: Si pruebas en un servidor de 16 núcleos y notas que al pasar del núcleo 8 al 16 la eficiencia se desploma, menciona la arquitectura del procesador. Usualmente es porque esos núcleos extra son "lógicos" (Hyper-Threading) y no núcleos físicos independientes, por lo que se pelean por la misma memoria física caché L1/L2.
3. Diferencia Integral vs Área: En la defensa, compara las curvas de las dos implementaciones. Generalmente, el Área 2D requiere generar más números aleatorios (dos por dardo) y una condicional (if x^2+y^2). Fíjate si esa carga de cálculo local extra hace que aproveche mejor el paralelismo (oculta mejor las latencias de MPI) que la Integral 1D.
-->
