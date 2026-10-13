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

### Balanceo de Carga (Load Balancing)
* Implementamos un **Balanceo de Carga Estático**. Cada proceso recibe una porción matemática idéntica, y el "resto" de la división entera se distribuye equitativamente sumando una iteración extra a los primeros procesos. Decidimos esto pues el método de Monte Carlo es un problema de naturaleza *Embarrassingly Parallel* (Vergonzosamente Paralelo), el costo de evaluar la función objetivo es idéntico en cada iteración y, al no existir intercambio de mensajes ni consultas en tiempo real entre los hilos o procesos para pedir más tareas, los procesadores aprovechan el 100% de su ciclo de reloj en calcular. Un esquema Maestro-Trabajador hubiera introducido una cantidad masiva e innecesaria de comunicación entre los procesos (mensajes MPI de petición de tareas), arruinando la eficiencia.


### Elección del Generador Pseudoaleatorio (PRNG) y Reproducibilidad
* Inicialmente consideramos la implementación de la función clásica rand() de la biblioteca estandar de C que vimos en clase. Sin embargo, identificamos que, a gran escala (miles de millones), el generador agota su ciclo y comienza a repetir la misma secuencia de numeros. Esto sucede porque RAND_MAX también está en las cifras de los miles de millones (10^9 aprox).
 * Por esta razon, optamos por usar la familia drand48, estándar en POSIX, ya que, amplía el espacio de estados a 48 bits (10^14 aprox.) lo cual garantiza que el ciclo no se agote en corridas masivas, y genera directamente un numero punto flotante double, ahorrando costo de conversión y división en cada iteración. Además, al ser estándar POSIX de C (stdlib.h), compila directamente con mpicc en cualquier PC o clúster sin necesidad de compilar paquetes de terceros.
 * También nos planteamos la idea de la reproducibilidad de los resultados, por lo que reemplazamos el uso de time(NULL), que dependía del uso del reloj del sistema, y, en su lugar, implementamos un esquema de semillas reproducibles distanciadas, para poder comparar el resultado de los experimentos.


### Elección de nro. de simulaciones y procesos
* Inicialmente se hicieron las pruebas en **PC**, utilizando una unidad más que la máxima capacidad de procesos, es decir, en vez de 4 procesos, probamos 6. Esto fue posible con la función oversubscribe de MPI.
* Para definir el número de simulaciones (N), decidimos consultar con el profesor, y, tras varias pruebas, dejamos la escala de N como: 10^4, 10^6, 10^7 y 10^9, para PC. Esta decisión se tomó, teniendo en consideración lo máximo que podría aguantar una PC, por sobre el tiempo que nos llevaría el experimento.
* Para el **servidor**, en un principio, por recomendación del profesor, usamos la máxima capacidad (384 GB RAM y 96 cores), y decidimos usar la escala: 1,2,4,6,8,16,32,64,96,128. Estos numeros se tomaron para la primera toma de datos, que fue la del círculo, luego fue modificada, a una escala que toma al proceso 1 y del 8 al 96, cuya justificación se explica en la sección de limitaciones y mejoras.
* Para elegir el N que usaríamos en el servidor, decidimos agregar uno más al rango elegido para PC. agregamos 10^11 simulaciones. Esto por recomendación del profesor, y para un mejor analisis del comportamiento de la paralelización.
* En conclusión, los numeros elegidos para los experimentos, fueron decididos teniendo en cuenta, la máxima capacidad de los dispositivos, para poder garantizar la mayor cantidad de datos que eran posibles de recoger. Además, se consideró un límite de tiempo de ejecución en el servidor; nuestro parametro fue que la ejecución de el programa con un solo proceso (serial), para el máximo N, no supere los 20 minutos.


### Metodología de Medición y Análisis
* Lo que hicimos fue implementar el Método de Monte Carlo de Integración y Cálculo de área en dos funciones/figuras distintas, haciendo uso de la paralelización con MPI, para analizar su eficiencia. Este proceso se llevó a cabo en el servidor facilitado por la cátedra y en las PCs de las integrantes, para poder hacer un analisis y comparación de rendimiento entre los dispositvos.
* Utilizamos la función **MPI_Reduce** que realiza una operación de reducción global (como puede ser calcular el máximo, la suma, hacer un AND lógico, etc) sobre cada uno de los miembros del grupo. La operación de reducción puede ser tanto una predefinida de la lista de operaciones como una operación definida por el usuario. En nuestro caso la utilizamos para el cálculo de la función en N puntos entre a y b para la integral, y para la generación de los N puntos de pruba para el método de Aceptación/Rechazo.
* Se utilizó un único MPI_Barrier previo a la captura del tiempo inicial con MPI_Wtime(). Si bien, no es necesario para nuestro programa, ya que la operación de reducción es bloqueante de por sí, se tomó esta desición con el fin de aportar buenas practicas y rigurosidad al trabajo, ya que, la operación, garantiza la consistencia del benchmark al sincronizar la largada de todos los procesos distribuidos, evitando sesgos por el retardo de inicialización del runtime de MPI. No se incluyeron barreras adicionales dentro del bucle de simulación para preservar el asincronismo y evitar el degradamiento por el Efecto del Eslabón Más Lento (Straggler Effect). 
* **Medición del tiempo**: A la hora de tomar los tiempos de ejecución de la paralelización, surgió el planteo de cuándo deberímos tomar el tiempo de fin (el tiempo de inicio era claro). La idea original fue la de hacerlo justo después de la operación de reducción,es decir, medir estrictamente el algoritmo paralelo. Pero, se nos ocurrió que, el cálculo que se hace dentro del proceso raíz, podría afectar al tiempo total y ser significativo. Luego de investigar en foros, decidimos optar por la primera alternativa, ya que es la regla metodológica obligatoria en la comunidad de HPC, y el cálculo es trabajo serial secundario del proceso 0 y no significa un costo significativo de tiempo.



## 📊 Análisis de los Resultados

En base a los fundamentos de HPC y de los métodos estocásticos, a la hora de interpretar las gráficas obtenemos los siguientes comportamientos esperados:
### **Convergencia del Error Absoluto:**
Por la Ley de los Grandes Números, a medida que $N$ aumenta, el error debe decrecer de forma inversamente proporcional a la raíz cuadrada de $N$ ($1/\sqrt{N}$). En nuestra gráfica Log-Log, esto se debe observar como una línea recta descendente y constante.

**Resultados**

![convergencia estocástica del círculo](image.png)

Podemos notar en esta gráfica que analiza los datos arrojados por el servidor, cómo se comporta de manera esperada, hasta las 10^9 simulaciones, donde, el cálculo de la integral sufre un estancamiento.
El error deja de disminuir, aunque no aumenta, se mantiene constante. 

Esto no sucede para el Area, que si baja el error absoluto a medida que aumenta N.


### **Speedup Real vs Ideal:** 
El Speedup Ideal traza una línea recta (donde $P$ procesos reducen el tiempo $P$ veces). En la realidad, la línea de nuestro código correrá por debajo de la ideal. La separación entre ambas líneas representa el *overhead* (el costo de inicializar MPI, balancear variables, y la comunicación final en el `MPI_Reduce`).

 **Resultados**

![speedup vs proceso: circulo](image-1.png)

Se observa claramente, como para un N muy pequeño (10⁴) la paralelización es ineficiente y no debería ser considerada para el método. 

Luego,para el resto de N, se observa que hasta los 64 procesos, tienen un comportamiento mas o menos ideal, es decir, el Speedup aumenta. 

Los resultados son los esperados, a partir de 64 procesos comienza a notarse un estancamiento del Speedpp. Es decir, el crecimiento es poco o directamente se mantiene constante, mientras que, para los N más pequeños hay, incluso, una disminusión significativa del Speedup.

En este punto, es interesante explicar un poco la arquitectura del Hardware del servidor. Entre 1 y 64 procesos, todos los núcleos residen en el Socket 0. La comunicación es local al chip. Al pasar de 64 procesos, MPI se ve obligado a utilizar el Socket 1. La comunicación ya no viaja por el silicio local, sino a través del bus inter-socket AMD Infinity Fabric. 


### ¿Con cuántos procesos conviene correr cada N?

En un principio, la intuición nos haría pensar que mientras más procesos, más rápido y mejor funciona el programa. Luego de llevar a cabo los experimentos para el proyecto, podemos pensar que el Speedup es mejor a medida que el nro de procesos aumenta de manera proporcional con N. 

La siguiente gráfica nos aclara las cosas:

![mejor N vs np](image-4.png)

Podemos observar que, efectivamente, para N=10⁴ el mejor Speedup se logra con 16 procesos, mientras que, 10⁶ simulaciones necesita 64. A partir de 10⁷, el mejor speedup se consigue con 96-128 procesos.

### **Eficiencia:**
La eficiencia inicia en 1.0 (o 100%) para 1 proceso. Debería ir decayendo levemente a medida que agregamos núcleos. Esta caída es consecuencia de que el bus de memoria caché se satura cuando todos los procesadores intentan consultar datos o escribir variables simultáneamente.

**Resultados**

![eficiencia paralela: circulo](image-3.png)

En nuestras gráficas se puede apreciar que el algoritmo paraleliza casi perfectamente (con una eficiencia de aprox 1) hasta 8-16 procesos para N grande, llegando hasta 32 en el cálculo de la integral.

### Comparación de Rendimiento: PC Local vs Servidor HPC
![pc vs servidor](image-5.png)
1. El servidor (representado por líneas verdes) demuestra un escalamiento óptimo.
   - Speedup: Crece de manera lineal, acompañando a la línea ideal
   - Eficiencia: Se mantiene estable y cercana a 1, esto se debe a la buena distribución de carga para los primeros 8 procesos.
2. El PC sufre limitaciones de arquitectura.
   - Escala de forma razonable hasta 4 procesos, debido a que posee 4 núcleos físicos.
3. Aceleración Superlineal (Efecto Caché)
   El fenómeno más destacable del experimento ocurre en el cálculo de **Área 2D** para la PC ejecutando la carga de datos más pesada (N=109, línea violeta continua) al usar **2 procesos MPI** 
   - El Speedup supera la diagonal teórica, logrando una **Eficiencia mayor a 1.0** (superior al 100%). 
   - Esto se conoce como **aceleración superlineal**. Al particionar el bloque masivo de 10^9 elementos entre 2 procesos, los conjuntos de datos resultantes logran encajar de manera mucho más óptima dentro de la memoria caché local de los núcleos del procesador. 
   - Al resolver el problema accediendo a la caché rápida en lugar de ir constantemente a la memoria RAM (más lenta), el procesador ahorra tanto tiempo que supera la aceleración teóricamente esperada por simplemente sumar un proceso extra. 


## Limitaciones y Mejoras Futuras

A lo largo del proceso de investigación y prueba tomamos desiciones y descubrimos limitaciones o mejoras que se pueden resolver/implementar a futuro.

### Elección del Generador Pseudoaleatorio (PRNG):
En nuestro código base priorizamos la portabilidad estándar POSIX con drand48(). Para una futura versión, la mejora recomendada es migrar a generadores basados en contadores como Philox (Random123) o bibliotecas especializadas como SPRNG.

### Elección de N y procesos
En la sección donde explicamos por qué tomamos los numeros que tomamos, se aclara que hubo una reducción en el numero de procesos tomados, para la segunda figura estudiada, que fue la parábola.

Cuando hicimos las pruebas con los valores originales, la ejecución, en total, nos tomó 12 horas. Lo que, consultando con el profe y debatiendo en grupo, concluímos que era demasiado tiempo. Por lo que, se limitó la máxima capacidad de computo, para que el experimento se lleve a cabo en un tiempo más corto. Consideramos esta desicion la más óptima para este proyecto, ya que no afecta en gran escala a las conclusiones finales.

Por último, se decidió tomar 5 repeticiones por nro de procesos, por la misma razón de optimizar tiempos.

Para una futura versión, la mejora recomendada es medir ambas figuras con la misma cantidad de procesos, o bajar el nro de N, y aumentar la cantidad de repeticiones, para obtener un conjunto de datos más robustos.

## Bibliografía
- [Dynamic Load Balancing of Parallel Monte Carlo Transport Calculations](https://www.osti.gov/biblio/15015938)
- [drand48](https://pubs.opengroup.org/onlinepubs/007904975/functions/drand48.html)
- [MPI_Reduce](https://lsi2.ugr.es/jmantas/ppr/ayuda/mpi_ayuda.php?ayuda=MPI_Reduce)
- [When do I need to use MPI_Barrier()?](https://stackoverflow.com/questions/13305814/when-do-i-need-to-use-mpi-barrier)
- [MPI global execution time](https://stackoverflow.com/questions/5298739/mpi-global-execution-time)
- [How do I interpret the results from MPI_Wtime()?](https://scicomp.stackexchange.com/questions/29876/how-do-i-interpret-the-results-from-mpi-wtime)
- [¿Qué es la Ley de Amdahal?](https://builtin-com.translate.goog/articles/amdahls-law?_x_tr_sl=en&_x_tr_tl=es&_x_tr_hl=es&_x_tr_pto=tc)
<!-- 
💡 CONSEJOS Y RECOMENDACIONES OCULTAS PARA TU DEFENSA ORAL: 
1. Anomalías con N pequeños (ej. N=10.000): Si observas que con 10.000 simulaciones usar 8 procesos demora MÁS TIEMPO que usar 1 proceso (Speedup menor a 1), ¡no te asustes y úsalo a tu favor! Explícale al profesor que "para problemas diminutos, el costo de coordinar y enviar mensajes entre los nodos de MPI demora más tiempo que simplemente resolver la matemática localmente".
2. Caídas bruscas de Eficiencia: Si pruebas en un servidor de 16 núcleos y notas que al pasar del núcleo 8 al 16 la eficiencia se desploma, menciona la arquitectura del procesador. Usualmente es porque esos núcleos extra son "lógicos" (Hyper-Threading) y no núcleos físicos independientes, por lo que se pelean por la misma memoria física caché L1/L2.
3. Diferencia Integral vs Área: En la defensa, compara las curvas de las dos implementaciones. Generalmente, el Área 2D requiere generar más números aleatorios (dos por dardo) y una condicional (if x^2+y^2). Fíjate si esa carga de cálculo local extra hace que aproveche mejor el paralelismo (oculta mejor las latencias de MPI) que la Integral 1D.
-->
