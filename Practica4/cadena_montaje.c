/**
 * @brief Implementa el ejercicio cadena_montaje.c de mensajes
 * @file cadena_montaje.c
 * @author Andres Salas andres.salas@estudiante.uam.es
 * @author Antonio Martin antonio.martinmasuda@estudiante.uam.es
 * @note Grupo 2202
 * @version 1.0
 * @date 8/04/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

/**
* @brief Definicion de la clave
*/
#define KEY 1300

/**
* @brief Definicion de la clave de fichero
*/
#define FILEKEY "/bin/cat"

/**
* @brief Definicion del numero de procesos hijos totales
*/
#define NUM_PROCESOS 3

/**
* @brief Definicion del tamanyo del "trozo" de lectura del fichero
*/
#define TROZOS 1



int k = 0;

/**
 * @brief Estructura mensaje que contiene todos sus parametros necesarios para la
 * realización del ejercicio con colas de mensajes
 */
typedef struct _Mensaje{
	long id; /*!<Tipo de mensaje*/
	/*Informacion a transmitir en el mensaje*/
	int valor; 
	char aviso[80];
} mensaje;

/**
* @brief funcion principal que pide un fichero de entrada y otro
*   de salida y realiza el paso de mensajes pedidos
* @param argc: contiene el número de parámetros totales pasados
* @param argv: contiene los parámetros pasados por el usuario
* @return int: valor de exito (OK) o fracaso (ERROR)
*/
int main(int argc, char* argv[]){
    /*
    * Declaración de variables
    */
    char *origen;
    char *destino;
    char aux[TROZOS];
    char aux2[TROZOS];
    int i,j,l;
    int pid = 0;
    int msqid;
    struct msqid_ds *buf = NULL;
	mensaje msg;
    key_t key;
    FILE* fo = NULL;
    FILE* fd = NULL;
    
    /*
    * Comprobacion de errores
    */

    if (argc == 2){
    	destino = "destino.txt";
    }

    else if(argc != 3){
        printf("Se debe pasar dos argumentos que seran fichero de origen y de destino\n");
        exit(EXIT_FAILURE);
    }

    else {
	destino = argv[2];
    }

    origen = argv[1];

    /* Reservamos memoria para la estructura msqid_ds */
    buf = (struct msqid_ds*) malloc(sizeof(struct msqid_ds));
    if(buf == NULL){
    	printf("Error al reservar memoria");
    	exit(EXIT_FAILURE);
    }

    /*  Obtenemos la clave para poder crear la cola de mensajes */
    key = ftok(FILEKEY, KEY);
    if(key == -1){
    	perror("Error al obtener key\n");
    	free(buf);
    	exit(EXIT_FAILURE);
    }
    /* Creacion de la cola de mensajes */
    msqid = msgget(key, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
    if(msqid == -1){
    	msqid = msgget(key, IPC_CREAT | SHM_R | SHM_W);
        if(msqid == -1){
            perror("Error al crear la cola de mensajes\n");
            free(buf);
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < NUM_PROCESOS-1; i++){
        if(pid == 0){        
            if((pid = fork()) == -1){
                printf("Error en el fork\n");
                msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
				exit(EXIT_FAILURE);
            }
        } 
        wait(NULL);

        /* If para lo que hará el proceso hijo A*/
        if(i == 1 && pid == 0){
        	fo = fopen(origen, "r");
        	if(fo == NULL){
        		printf("Error al abrir el fichero.\n");
        		msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
        		exit(EXIT_FAILURE);
    		}
    		while(fscanf(fo, "%s", aux) > 0){
    			msg.id = 1; /*Tipo de mensaje*/
				msg.valor= 0;
				strcpy (msg.aviso, aux);
				printf("%s\n", msg.aviso);
				msgsnd (msqid, (struct msgbuf *) &msg, sizeof(mensaje) - sizeof(long), IPC_NOWAIT);
				printf("%s\n", msg.aviso);
				memset(aux, 0, sizeof(aux));
			}
    		fclose(fo);
    		exit(EXIT_SUCCESS);
    	}

    	/* Else If para lo que hará el proceso hijo B*/
    	else if(i == 1 && pid > 0){
    		if(msgctl(msqid, IPC_STAT, buf) == -1){
    			printf("Error obtener la estructuras de la cola de mensajes %d.\n", errno);
        		msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
        		exit(EXIT_FAILURE);
    		}
    		for (k = buf->msg_qnum; k > 0; k--){
    			msgrcv (msqid, (struct msgbuf *) &msg, sizeof(mensaje) - sizeof(long), 1, 0);
    		
    			printf("%s\n", msg.aviso);
			
    			for(j=0;msg.aviso[j] != '\0';j++){
	    			if(msg.aviso[j] < 'a' || msg.aviso[j] > 'z')
    					continue;
    				msg.aviso[j] = (int) msg.aviso[j] - 32;
    			}
			
    			msg.id = 2;
    			msgsnd (msqid, (struct msgbuf *) &msg, sizeof(mensaje) - sizeof(long), IPC_NOWAIT);
    			printf("%s\n", msg.aviso);
    		}
    		exit(EXIT_SUCCESS);
    	}

    	/* Else If para lo que hará el proceso hijo C*/
    	else if(i == 0 && pid > 0){
			fd = fopen(destino, "w");
        	if(fd == NULL){
        		printf("Error al abrir el fichero.\n");
        		free(buf);
        		msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
        		exit(EXIT_FAILURE);
    		}
    		if(msgctl(msqid, IPC_STAT, buf) == -1){
    			printf("Error obtener la estructura de la cola de mensajes.\n");
        		msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
        		free(buf);
        		exit(EXIT_FAILURE);
    		}
    		for (k = buf->msg_qnum; k > 0; k--){
	    		msgrcv (msqid, (struct msgbuf *) &msg, sizeof(mensaje) - sizeof(long), 2, 0);
			
				printf("%s\n", msg.aviso);
    		
				for(j=0;msg.aviso[j] != '\0';j++){
					aux2[j] = msg.aviso[j];  
				}
			
				fprintf(fd, "%s", aux2);
				memset(aux2, 0, sizeof(aux2));
			}
			fclose(fd);
			msgctl (msqid, IPC_RMID, buf);
			free(buf);
    		exit(EXIT_SUCCESS);
    	}
	}
	free(buf);
	msgctl (msqid, IPC_RMID, buf);
    exit(EXIT_SUCCESS);
}
