#define _POSIX_C_SOURCE 200809L //nos permite usar funciones POSIX como nanosleep
#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h> //para poder manejar el txt
#include <time.h> //para el rand

struct actividad{

    char ID[10]; //Id alfanumerico, asi que puede ser letras y numeros
    char nombre[50]; //nombre actividad
    int tiempo; //ms de la actividad, 100 a 5000ms si no esta asignado
    int dependencias_contador; //contador de dependencias
    char dependencias[20][10]; //arreglo bidimensional para guardar los IDs de las dependencias, ademas asumimos que una actividad tendria como maximo 20 dependencias
    int estado; // estado de la actividad. pendiente = 0, corriendo = 1, finalizado = 2, error = -1
    pid_t pid; // pid del proceso hijo
    int dependientes_contador; // Cuantas actividades dependende de esta actividad
    int dependientes[100]; // guardamos los indices de quienes dependen de esta actividad, asumiendo que una actividad no tendra +100 dependientes

};

int obtener_linea(char* linea, struct actividad *acti){ //usamos int para verificar extracción correcta

    char *pos = strchr(linea, ':'); //Posición del atributo

    if(pos == NULL){

        printf("Linea mal formada\n");
        return -1;

    }

    int contador = sscanf(linea, "%9s : %49s : %d", acti->ID, acti->nombre, &acti->tiempo);
    //se usan numeros para los %s considerando el tamaño del arreglo. y &acti porque no es un arreglo y sscanf es una función externa

      if(contador < 2){ //En caso de que venga una linea del txt con formato diferente

        printf("Linea mal formada\n");
        return -1;

    }

    if(contador < 3){ //en caso de que la tarea no tenga tiempo de ejecucion

        acti->tiempo = (rand() % 4901) + 100;
        //(rand() % (max - min + 1)) + min

    }

    char *resto = strchr(pos + 1, ':'); // resto = posición del 2do :

    if(resto != NULL){

        resto = strchr (resto + 1, ':'); // resto = posición del 3er :

    }

    //inicializar dependencias_contador y estado
    acti->dependencias_contador = 0;
    acti->estado = 0;

    if (resto != NULL) {
        resto++; //saltamos el 3er : para que resto apunte a las dependencias

        char *fragmento = strtok(resto, ", \n");

        while(fragmento != NULL){ //Sera NULL cuando no hayan mas dependencias

            if(acti -> dependencias_contador >= 20){ //Para evitar salirnos de la matriz dependencias

                printf("Desborde de dependencias permitidas\n");
                return -1;

            }

            // Copiamos el ID de la dependencia directamente a la matriz
            strcpy(acti->dependencias[acti->dependencias_contador], fragmento);
            acti->dependencias_contador++; //aumentamos en 1 la cantidad de dependencias

            fragmento = strtok(NULL, ", \n"); //siguiente fragmento
        }
    }
    return 0; 
}

// Función que verifica si todas las dependencias de una tarea están en estado 2 (Finalizadas)
int dependencias_finalizadas(struct actividad *tarea, struct actividad tareas[], int total_tareas) {
    for (int i = 0; i < tarea->dependencias_contador; i++) {
        char *id_tarea = tarea->dependencias[i];
        int estado_dependencia = 0;

        // Buscamos la dependencia (tarea) en el arreglo de tareas
        for (int j = 0; j < total_tareas; j++) {
            if (strcmp(tareas[j].ID, id_tarea) == 0) {
                estado_dependencia = tareas[j].estado;//id de la tarea encontrada
                break;
            }
        }
        
        // Si encontramos una sola dependencia que no está terminada (estado 2), retornamos 0 (falso)
        if (estado_dependencia != 2) {
            return 0; 
        }
    }
    return 1; // Todas las dependencias están finalizadas (o no tiene dependencias)
}

int siguiente_tarea_lista(struct actividad tareas[] , int total_tareas){

    for(int i = 0; i<total_tareas; i++){

        //una tarea estara lista para ejecutar cuando este en estado 0 y sus dependencias finalizadas (estado 2)

        if(tareas[i].estado == 0 && dependencias_finalizadas(&tareas[i], tareas, total_tareas)){

            return i;
            //Retorna la tarea que cumpla la condición para ser ejecutada.

        }

    }

    return -1; //No se encontro tarea que cumpla la condición

}
//función para buscar en el arreglo de tareas su pid
int buscar_pid_tarea(pid_t pid, struct actividad tareas[], int total_tareas){

    for(int i =0; i< total_tareas; i++){

        if(tareas[i].pid == pid){ //se compara el pid de la tarea con el pid que se recibe de parametro

            return i; //se encontro la tarea y se retorna su indice

        }

    }

    return -1; //en caso de no encontrar una tarea con el pid de los parametros puestos en la función, pero no deberia ocurrir
}

int ejecutar_tarea(struct actividad *tarea, int write_fd){

    pid_t pid = fork();

    if(pid < 0){

        printf("Error al crear proceso\n");
        return -1;

    }

    if(pid==0){ //proceso hijo

        struct timespec pausa; //debemos usar un struct timespect, ya que nanosleep lo recibe como parametro y sleep() solo funciona en segundos

        pausa.tv_sec = tarea->tiempo / 1000; // pasamos los ms a segundos  
        pausa.tv_nsec = (tarea->tiempo % 1000) * 1000000L; //obtenemos el resto de los segundos y lo pasamos a nanosec, L para variable long
        nanosleep(&pausa, NULL); //segundo parametro en NULL, ya que no nos interesa saber cuando le faltaba en caso de una señal

        // paso de mensajes (pipe): Enviamos el ID de la tarea al padre al finalizar
        write(write_fd, tarea->ID, strlen(tarea->ID) + 1);

        _exit(0); //_exit y no exit, para evitar que se dupliquen los buffer de salida que heredo del proceso padre

    }

    //para el proceso padre
    tarea->pid = pid;
    tarea->estado = 1; //estado = 1: corriendo

    return 0;
}

//esta función es para almacenar que actividades dependen de cierta tarea
void crear_dependientes(struct actividad tareas[], int total_tareas){

    //inicializamos dependendientes en 0
    for(int i = 0; i< total_tareas; i++){

        tareas[i].dependientes_contador = 0;

    }

    //recorremos cada actividad primero

    for(int i = 0; i< total_tareas; i++){

        //recorremos las dependencias de cada actividad

        for(int j = 0; j<tareas[i].dependencias_contador; j++){

            char* id_dependencia = tareas[i].dependencias[j];

            for(int z = 0; z< total_tareas; z++){

                if(strcmp(tareas[z].ID, id_dependencia)  == 0){//comparamos los ID, 0 si son iguales

                    if(tareas[z].dependientes_contador >= 100){

                        printf("Muchas tareas dependen de esta actividad \n"); //Considerando que una actividad no puede tener mas de 100 dependientes
                        break; //si tiene +100 dependientes, solo se guardaran 100

                    }

                     tareas[z].dependientes[tareas[z].dependientes_contador] = i;
                     tareas[z].dependientes_contador++;
                    //finalizamos de guardar dependientes
                    break;

                }                

            }

        }

    }

}


int main(int argc, char* argv[]){

    srand(time(NULL));//para que rand no genere siempre lo mismo

    if(argc != 3){//input correcto

         printf("Usar: %s <archivo.txt> <K>\n", argv[0]);
         return 1;
    }
    
    
    int k = atoi(argv[2]); //Cantidad maxima de procesos a tener

    char *nombre_archivo = argv[1];

    FILE *archivo = fopen(nombre_archivo, "r");//abrir archivo con nombre existente, "r" es para solo lectura (read)

    if (!archivo) {

        perror("Error al abrir archivo\n");
        return 1;

    }

    char linea[512]; //bytes
    static struct actividad tareas[10000]; // Arreglo para guardar las actividades del DAG, static en caso de que agrandandemos la matriz
    int total_tareas = 0;

    while (fgets(linea, sizeof(linea), archivo) != NULL) { //leer cada linea del archivo
        //ignorar líneas vacías o saltos de línea sueltos
        if (strlen(linea) <= 1) continue;

        //llamamos a la función de obtener linea
        if (obtener_linea(linea, &tareas[total_tareas]) == 0) {
            total_tareas++;
        }
    }

    fclose(archivo);//cerrar el archivo

    //se imprime las tareas para verificar que funciona bien la lectura con las dependencias correspondientes
    printf("\nTotal tareas: %d\n", total_tareas);
    for (int i = 0; i < total_tareas; i++) {
        printf("ID: %s | Nombre: %s | Tiempo: %d ms | Dependencias: ", 
               tareas[i].ID, tareas[i].nombre, tareas[i].tiempo);
        for (int j = 0; j < tareas[i].dependencias_contador; j++) {
            printf("[%s] ", tareas[i].dependencias[j]);
        }
        printf("\n");
    }

    crear_dependientes(tareas, total_tareas); //añadimos los dependientes de cada tarea

    //BLOQUE DE PRUEBA PARA VER QUE SE AÑADA BIEN LOS DEPENDIENTES DE CADA TAREA

    for(int i =0; i< total_tareas; i++){

        printf("La tarea: %s tiene %d dependientes", tareas[i].ID, tareas[i].dependientes_contador);

        for(int j=0; i< tareas[i].dependientes_contador; j++){

            int id = tareas[i].dependientes[j];
            printf("[%s] ", tareas[id].ID);

        }
        printf("\n");

    }

    //FIN BLOQUE PRUEBA
    int hijos_activos = 0;
    int tareas_completadas = 0;

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Error al crear el pipe");
        return 1;
    }
    
    while(tareas_completadas< total_tareas){

        //vamos creando procesos hasta un limite de K procesos

        while(hijos_activos < k){

            int indice_tarea = siguiente_tarea_lista(tareas, total_tareas);

            if(indice_tarea == -1){ //tarea no lista para ejecutarse aun

                break;

            }
            //tarea lista para ejecutarse
            ejecutar_tarea(&tareas[indice_tarea], pipe_fd[1]);
            hijos_activos++; //incrementamos la cantidad de hijos activos ejecutandose a la vez

        }

        //esperamos a que algun proceso hijo termine su ejecucion

        char id_terminado[10];
        //el padre lee el mensaje enviado por el hijo a través del pipe
        read(pipe_fd[0], id_terminado, sizeof(id_terminado));

        //esperamos al proceso hijo para evitar zombies
        int estado;
        pid_t proceso_terminado = waitpid(-1, &estado, 0);

        if(proceso_terminado > 0){ //si -1 = error y 0 es un caso no posible

            int indice_tarea_terminada = buscar_pid_tarea(proceso_terminado, tareas, total_tareas);

            if(indice_tarea_terminada != -1){ //no deberia dar -1 pero es caso borde

                tareas[indice_tarea_terminada].estado = 2; //tarea finalizada
                printf("Tarea ID: %s finalizada\n", tareas[indice_tarea_terminada].ID);
                
            }

            hijos_activos--;
            tareas_completadas++;

        }

    }

    return 0;

    // //INICIO BLOQUE DE PRUEBA

    // int indice_tarea;

    // while( (indice_tarea = siguiente_tarea_lista(tareas, total_tareas) ) != -1 ){

    //     printf("Ejecutando la tarea ID: %s Con nombre: %s\n", tareas[indice_tarea].ID, tareas[indice_tarea].nombre);
    //     tareas[indice_tarea].estado = 2;

    // }

    // //FIN BLOQUE DE PRUEBA



    // for(int i = 0; i <k-1 ; i++){

    //     pid_t pid = fork();

    //     if(pid<0){

    //         printf("Error al crear procesos\n");
    //         return 1;
        
    //     }else if(pid>0){ //Proceso padre

    //         printf("Proceso: %d\n", i);
    //         wait(NULL);
    //         break;

    //     }else{ //Proceso hijo

    //         printf("Proceso: %d\n", i+1);
    //         continue;

    //     }

    //     //Estaba pensando en hacer procesos en cascada pero no se si estara bueno
    //     //pq hay procesos que necesitan que otros terminen, asi que no estoy muy sesguro que funcione con esa estructura

    // }


    return 0;
}