# Planificador Dieciochero - Tarea 1 Sistemas Operativos
### Integrantes: Alexander Bravo e Isaías Vásquez
Planificador de actividades modelado como un Grafo Acíclico Dirigido (DAG), desarrollado en C utilizando control de procesos mediante `fork`, comunicación asíncrona por tuberías (`pipes`) y manejo de señales.

## Funciones Implementadas

* **`obtener_linea()`**: Procesa cada línea del archivo `plan.txt`, extrayendo el ID alfanumérico, el nombre de la actividad, la duración y las dependencias. Si el tiempo no está especificado, le asigna de forma aleatoria un valor entre 100 y 5000 ms usando `rand()`.
* **`siguiente_tarea_lista()` y `dependencias_finalizadas()`**: Gestionan la travesía del DAG. Verifican de manera iterativa que una tarea se encuentre en estado pendiente (`0`) y que todas sus dependencias hayan concluido exitosamente (`estado = 2`) antes de habilitarla para su ejecución.
* **`ejecutar_tarea()`**: Crea un nuevo proceso hijo mediante `fork()`. En el hijo, configura la pausa de tiempo con `nanosleep()` (usando `struct timespec`) y reporta el término al padre escribiendo el ID en el extremo de escritura del `pipe`.
* **`buscar_pid_tarea()` y `crear_dependientes()`**: Mapean las relaciones de dependencia inversa y permiten enlazar el PID del proceso hijo finalizado con su respectiva actividad en el arreglo estático.
* **`manejar_ctrl_c()`**: Función asociada al manejador de la señal `SIGINT` (`Ctrl+C`) que intercepta la interrupción en el proceso padre e invoca `kill()` con `SIGKILL` para abortar de manera limpia y ordenada todas las actividades activas.

---
## Modo de Uso

### 1. Compilación
El programa debe compilarse en C y las advertencias obligatorias requeridas para entornos Unix:
``gcc -Wall -Wextra -std=c17 planificador.c -o planificador``

## Ejecución
El programa se ejecuta desde la terminal pasando como parámetros obligatorios el archivo de planificación y el límite máximo de concurrencia $K$:

``./planificador <archivo.txt> <K>``

## Justificación de Decisiones de Diseño

* **Modelado del DAG**: Se estructuró un sistema basado en un arreglo estático de gran capacidad que evalúa iterativamente las dependencias cruzadas mediante matrices de adyacencia e índices inversos (`crear_dependientes`), garantizando una travesía limpia y eficiente sin requerir estructuras complejas de memoria dinámica.
* **Control de Concurrencia ($K$)**: Se implementó un ciclo centralizado que restringe la creación de nuevos procesos hijos hasta respetar estrictamente el límite $K$, pausando la ejecución de nuevas tareas mediante un diseño libre de bloqueos activos (*busy-waiting*).
* **Paso de Mensajes (Pipes)**: Se utiliza una tubería unidireccional donde el proceso hijo escribe su ID al finalizar. Esto permite que el padre procese la información de manera asíncrona mediante `read()` y `waitpid()`, evitando *deadlocks* por comunicación directa entre múltiples hijos.
* **Ctrl+C (Seremi) y Manejo de Señales por Herencia**: Dado que los hijos heredan los manejadores de señales al invocar `fork`, se restableció el comportamiento por defecto (`SIG_DFL`) en los procesos hijos. De este modo, la señal `SIGINT` (`Ctrl+C`) es capturada exclusivamente por el proceso padre a través de `manejar_ctrl_c`, el cual ejecuta una limpieza segura de todos los procesos activos mediante `kill()`.
* **Aislamiento de Errores**: Gracias al modelo de procesos independientes de UNIX y al manejo de estados individuales (`estado = -1` o rama abortada), cualquier falla interna o terminación repentina en un hijo no compromete la estabilidad global del planificador principal ni de las ramas independientes del DAG.
* **Gestión de Tiempos y Asignación Aleatoria**: La simulación temporal se realiza mediante `nanosleep` utilizando la estructura `timespec`, lo que permite un control preciso de milisegundos y nanosegundos. Además, se integró una lógica condicional con `rand()` para asignar de forma dinámica duraciones predeterminadas (entre 100 y 5000 ms) en tareas que omiten este parámetro en el archivo de entrada.
