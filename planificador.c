#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){

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
    char linea[2];
    while (fgets(linea, sizeof(linea), archivo) != NULL) { //leer cada linea del archivo
        printf("%s", linea);
    }
    fclose(archivo);//cerrar el archivo

    for(int i = 0; i <k-1 ; i++){

        pid_t pid = fork();

        if(pid<0){

            printf("Error al crear procesos\n");
            return 1;
        
        }else if(pid>0){ //Proceso padre

            printf("Proceso: %d\n", i);
            wait(NULL);
            break;

        }else{ //Proceso hijo

            printf("Proceso: %d\n", i+1);
            continue;

        }

        //Estaba pensando en hacer procesos en cascada pero no se si estara bueno
        //pq hay procesos que necesitan que otros terminen, asi que no estoy muy sesguro que funcione con esa estructura

    }


    return 0;
}