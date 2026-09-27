#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h> //para poder manejar el txt

struct actividad{

    char ID[10]; //Id alfanumerico, asi que puede ser letras y numeros
    char nombre[50]; //nombre actividad
    int tiempo; //ms de la actividad, 100 a 5000ms si no esta asignado
    int dependencias_contador; //contador de dependencias
    int* dependencias; //arreglo dinamico para las dependencias
    int estado; // estado de la actividad. pendiente = 0, corriendo = 1, finalizado = 2, error = -1

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

    //inicializar dependencias_contador y dependencias

    acti->dependencias_contador = 0;
    acti->dependencias = NULL; //en NULL para evitar redimencionamiento a una dirección invalida

    if (resto != NULL) {

        resto++; //saltamos el 3er: para que resto apunte a las dependencias

        char *fragmento = strtok(resto, ", \n");

        while(fragmento != NULL){ //Sera NULL cuando no hayan mas dependencias

            //vamos agrandando el arreglo de dependencias
            int *temp = realloc( acti->dependencias, sizeof(int) * (acti->dependencias_contador + 1));

            if (temp == NULL) { //en caso de que se acabe la memoria

                perror("realloc");
                exit(EXIT_FAILURE);

            }

            acti->dependencias = temp; //nuevo tamaño para las dependencias
            acti->dependencias[acti->dependencias_contador] = atoi(fragmento); //guardamos la dependencia
            acti->dependencias_contador++; //aumentamos en 1 la cantidad de dependencias de la tareaa

            fragmento = strtok(NULL, ", \n"); //siguiente fragmento
        }

    }
    return 0; 
}



int main(int argc, char* argv[]){

    srand(time(NULL));//para que rand no genere siempre lo mismo

    if(argc != 3){//input correcto

         printf("Usar: %s <archivo.txt> <K>\n", argv[0]);
         return 1;
    }
    
    
    int k = atoi(argv[2]); //Cantidad maxima de procesos a tener

    char *nombre_archivo = argv[1];

    FILE *archivo = fopen(nombre_archivo, "r");//abrir archivo con nombre existente

    if (!archivo) {

        perror("El archivo no existe\n");
        return 1;

    }

    char linea[256]; //bytes

    while (fgets(linea, sizeof(linea), archivo) != NULL) { //leer cada linea del archivo

        printf("%s", linea);

    }

    fclose(archivo);//cerrar el archivo

    
    
    
    
    
    
    
    




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