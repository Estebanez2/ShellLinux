//Alejandro Estebanez Moreno

/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/
#include "parse_redir.h"
#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */


job *listaTareas = NULL;
int tareasSUSPENDED = 0;
int vecesC = 0;
int matados = 0;
int vecesMatados = 0;
int autokill = 0;

// FunciÃ³n para liberar la memoria de un string
void liberarString(char *str) {
    if (str != NULL) {
        free(str);
        str = NULL; // Opcional: para evitar el uso de punteros colgantes
    }
}

// FunciÃ³n para liberar la memoria de un array de strings
void liberarArrayDeStrings(char **array, int size) {
    if (array != NULL) {
        for (int i = 0; i < size; i++) {
            free((array)[i]);
        }
        free(array);
        array = NULL; // Opcional: para evitar el uso de punteros colgantes
    }
}

// FunciÃ³n para liberar la memoria de un int
void liberarInt(int *str) {
    if (str != NULL) {
        free(str);
        str = NULL; // Opcional: para evitar el uso de punteros colgantes
    }
}

void liberarJobRespawnable(jobRespawnable *jobRes) {
    if (jobRes == NULL) {
        return;
    }

    liberarArrayDeStrings(jobRes->args, jobRes->size);
    liberarString(jobRes->file_in);
    liberarString(jobRes->file_out);
    liberarInt(jobRes->mask);
    free(jobRes);
}

void comprobar_cd(char *args[])
{	
    // Si el comando es 'cd', cambiar al directorio especificado o al directorio HOME si no se especifica ninguno
    int cambioCarpeta = chdir(args[1] ? args[1] : getenv("HOME"));
    if (cambioCarpeta == -1)
    {
        perror("chdir");
    }
}

void comprobar_jobs(char *args[])
{
    block_SIGCHLD();
    print_job_list(listaTareas);	// Imprimir la lista de trabajos
    if(empty_list(listaTareas) == 1)
    {
        printf("La lista estÃ¡ vacÃ­a\n");	// Si la lista de trabajos estÃ¡ vacÃ­a, imprimir un mensaje
    }
    unblock_SIGCHLD();
}

void comprobar_currjob(char *args[])
{
    block_SIGCHLD();
    job *tarea;
    tarea = get_item_bypos(listaTareas, 1);
	if(tarea != NULL){
		printf("Trabajo actual: PID=%d command=%s\n", tarea->pgid, tarea->command);	
	} else {
		printf("No hay trabajo actual\n");	
	}
    unblock_SIGCHLD();
}

void comprobar_deljob(char *args[])
{
    block_SIGCHLD();
    job *tarea;
    tarea = get_item_bypos(listaTareas, 1);
	if(tarea != NULL){
		if(tarea->state == STOPPED){
			printf("No se permiten borrar trabajos en segundo plano suspendidos\n");	
		} else {
			printf("Borrando trabajo actual de la lista jobs: PID=%d command=%s\n", tarea->pgid, tarea->command);	
			delete_job(listaTareas, tarea);
		}
		
	} else {
		printf("No hay trabajo actual\n");	
	}
    unblock_SIGCHLD();
}

void traverse_proc(void) {
    DIR *d; 
    struct dirent *dir;
    char buff[2048];
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            sprintf(buff, "/proc/%s/stat", dir->d_name); 
            FILE *fd = fopen(buff, "r");
            if (fd){
                long pid;     // pid
                long ppid;    // ppid
                char state;   // estado: R (runnable), S (sleeping), T(stopped), Z (zombie)

                // La siguiente lÃ­nea lee pid, state y ppid de /proc/<pid>/stat
                fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid);
                if(state == 'Z'){
                	printf("%ld\n", pid);
                }
                fclose(fd);
            }
        }
        closedir(d);
    }
}



void comprobar_bg(char *args[]) 
{
    block_SIGCHLD();
    int posicion = 1;
    if(args[1] != NULL)
    {
        char *endptr;
        posicion = (int)strtol(args[1], &endptr, 10);	// Convertir la posiciÃ³n del trabajo de cadena a entero
    }
    
    job *tarea;
    tarea = get_item_bypos(listaTareas, posicion);	// Obtener el trabajo de la lista por posiciÃ³n
    
    if(tarea != NULL)
    {
        // Si el trabajo estÃ¡ detenido o es RESPAWNABLE, cambiar su estado a BACKGROUND y continuar su ejecuciÃ³n
        if(tarea->state == STOPPED || tarea->state == RESPAWNABLE)
        {
            tarea->state = BACKGROUND;
            killpg(tarea->pgid, SIGCONT);
        }
    }
    unblock_SIGCHLD();
}

void comprobar_fg(char *args[], int *booleanaFG, int *pid_fork) 
{
    block_SIGCHLD();
    int posicion = 1;
    if(args[1] != NULL)
    {
        char *endptr;
        posicion = (int)strtol(args[1], &endptr, 10);	// Convertir la posiciÃ³n del trabajo de cadena a entero
    }
    job *tarea = get_item_bypos(listaTareas, posicion);

    if (tarea != NULL) 
    {
        *booleanaFG = 1;	// Establecer el booleanaFG en 1 para indicar que se ejecuta en primer plano
        tcsetpgrp(STDIN_FILENO, tarea->pgid);	// Establecer el grupo de procesos de la terminal de entrada al del trabajo
        if (tarea->state == STOPPED)
        {
            killpg(tarea->pgid, SIGCONT);
        }
        
        *pid_fork = tarea->pgid;	// Establecer el PID del trabajo en el proceso hijo
        strcpy(args[0], tarea->command);
        delete_job(listaTareas, tarea);		// Eliminar el trabajo de la lista de trabajos
    } 
    unblock_SIGCHLD();
}


void *thread_function(void *arg) {
    job *tarea = (job *)arg;
    sleep(tarea->segundosDeVida);
    int kill_result = kill(tarea->pgid, 0);
    if (kill_result == 0 || (kill_result == -1 && errno != ESRCH)) {
        
        	kill(tarea->pgid, SIGKILL);
        	printf("Proceso aniquilado\n");
        
    } else {
        printf("El proceso ya ha terminado\n");
    }
    free(tarea); // Liberar la memoria de job
    pthread_detach(pthread_self()); // Marcar el hilo como detached
    return NULL;
}


void comprobar_alarmThread(char *args[], int *pid_fork, int segundosDeVida) 
{
    // Crear una nueva instancia de job y asignar los valores relevantes
    job *tarea = malloc(sizeof(job));
    if (tarea == NULL) {
        perror("Error al asignar memoria para job");
        exit(EXIT_FAILURE);
    }
    tarea->pgid = *pid_fork;
    tarea->command = args[0]; // Suponiendo que args[0] es un string vÃ¡lido
    tarea->state = BACKGROUND;
    tarea->next = NULL; // Ajustar segÃºn sea necesario
    tarea->args = args; // Suponiendo que args es un arreglo de cadenas vÃ¡lido
    tarea->respawnable = NULL; // Puedes ajustar esto si es necesario
    tarea->segundosDeVida = segundosDeVida; // Convertir el segundo argumento a entero
     
    // Crear el hilo
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, tarea) != 0) {
        perror("Error al crear el hilo");
        free(tarea); // Liberar la memoria de job
        exit(EXIT_FAILURE);
    }
}


void proceso_padre(int *pid_fork, int background, char *args[], int * booleanaFG, jobRespawnable *jobRes, int *segundosDeVida, int *segundosDelay) 
{
    int pid_wait;
    int status;
    enum status status_res;
    int info;
    
    if(*segundosDeVida != 0)
    {
    	int copiaSegundos = *segundosDeVida;
    	comprobar_alarmThread(args, pid_fork, copiaSegundos);
    	*segundosDeVida = 0;
    }
    
    
    if (background == 0) 
    {
    		
        tcsetpgrp(STDIN_FILENO, *pid_fork);	//Ceder la terminal
        	
        pid_wait = waitpid(*pid_fork, &status, WUNTRACED);	// Esperar al proceso hijo y obtener su estado
        	
        tcsetpgrp(STDIN_FILENO, getpgrp()); // Restaurar la terminal 
        if (pid_wait == -1)
        {
           	perror("waitpid");
           	exit(EXIT_FAILURE);
        }
        	
        status_res = analyze_status(status, &info);		// Analizar el estado del proceso hijo
        printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
        
        if(status_res == SUSPENDED) 	// Si el proceso hijo estÃ¡ suspendido, agregarlo a la lista de trabajos detenidos
        {
           	block_SIGCHLD();
           	job *tarea = new_job(*pid_fork, args[0], STOPPED);
           	if (tarea == NULL) 
        	{
           		perror("new_job");
           		exit(EXIT_FAILURE);
        	}
           	add_job(listaTareas, tarea);
           	tareasSUSPENDED++;
        	printf("Tareas SUSPENDED: %d\n", tareasSUSPENDED);
           	unblock_SIGCHLD();
        }
        *booleanaFG = 0;
    	
    	
    } 
    else if(background == 1)
    {	
    	
    	block_SIGCHLD();
        job* tarea = new_job(*pid_fork, args[0], BACKGROUND);	// Crear un nuevo trabajo en segundo plano
        if (tarea == NULL) 
        {
            perror("new_job");
            exit(EXIT_FAILURE);
        }
        
        add_job(listaTareas, tarea);	// Agregar el trabajo a la lista de trabajos
        printf("Background job running... pid: %d, command: %s\n", *pid_fork, args[0]);
        unblock_SIGCHLD();
    }
    else if(background == 2)
    {
    	
    	block_SIGCHLD();
    	
    	job* tarea = new_job(*pid_fork, args[0], RESPAWNABLE);	// Crear un nuevo trabajo RESPAWNABLE
    	if (tarea == NULL) 
        {
            perror("new_job");
            exit(EXIT_FAILURE);
        }
        
        tarea->respawnable = jobRes;	// Asignar el puntero a la estructura jobRespawnable al trabajo RESPAWNABLE
        add_job(listaTareas, tarea);	// Agregar el trabajo a la lista de trabajos
        printf("Background job running (RESPAWNABLE)... pid: %d, command: %s\n", *pid_fork, args[0]);
        unblock_SIGCHLD();
    }
}


void proceso_hijo(char *args[], char *file_in, char *file_out, int  mask_count, int *mask_array) 
{
    FILE *entradaEstandar, *salidaEstandar;
    entradaEstandar = stdin;
    salidaEstandar = stdout;

    restore_terminal_signals();	// Restaurar las seÃ±ales del terminal al estado por defecto
    
    if(mask_count != 0)
    {
     
    	for(int i=0; i<mask_count; i++){
    		mask_signal(mask_array[i], SIG_BLOCK);
    	}
    }
    
   	setpgid(0, 0);	// Establecer el grupo de procesos del proceso hijo igual al PID del proceso hijo

    // Si se especifica un archivo de entrada, abrirlo para lectura
    if (file_in != NULL) 
    {
        entradaEstandar = fopen(file_in, "r");
        if (entradaEstandar == NULL) 
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    // Si se especifica un archivo de salida, abrirlo para escritura
    if (file_out != NULL) 
    {
        salidaEstandar = fopen(file_out, "w");
        if (salidaEstandar == NULL) 
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

	
	
    
    dup2(fileno(entradaEstandar), fileno(stdin));	// Redirigir la entrada estÃ¡ndar y la salida estÃ¡ndar segÃºn los archivos especificados
    dup2(fileno(salidaEstandar), fileno(stdout));
   	execvp(args[0], args);	// Reemplazar la imagen del proceso hijo con un nuevo programa
    dup2(STDERR_FILENO, STDOUT_FILENO);	// En caso de error en execvp, redirigir la salida de errores estÃ¡ndar a la salida estÃ¡ndar
    
    //if (file_in != NULL) 
    //{
    //    fclose(entradaEstandar);
    //}

    //if (file_out != NULL) 
    //{
    //    fclose(salidaEstandar);
    //}

	// No hace falta hacer fclose porque el exit ya los cierra

    perror("execvp");	 // Imprimir mensaje de error en caso de fallo en execvp
    exit(EXIT_FAILURE);	// Salir con un cÃ³digo de error
}


void parse_mask_command(char *args[], int *mask_count, int **mask_array, char ***command_args) {
    int i = 1;
    *mask_count = 0;
    *mask_array = NULL;
    *command_args = NULL;

    // Contar el nÃºmero de elementos en mask
    while (args[i] != NULL && strcmp(args[i], "-c") != 0) {
        (*mask_count)++;
        i++;
    }

    // Asignar y llenar el array de mask
    if (*mask_count > 0) {
        *mask_array = (int *)malloc((*mask_count) * sizeof(int));
        if (*mask_array == NULL) {
            perror("Error al asignar memoria para mask_array");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < *mask_count; j++) {
            (*mask_array)[j] = atoi(args[j + 1]);
        }
    }

    // Asignar y llenar el array de command_args
    if (args[i] != NULL && strcmp(args[i], "-c") == 0) {
        i++; // Saltar el "-c"
        int command_args_count = 0;
        int k = i;
        while (args[k] != NULL) {
            command_args_count++;
            k++;
        }

        *command_args = (char **)malloc((command_args_count + 1) * sizeof(char *));
        if (*command_args == NULL) {
            perror("Error al asignar memoria para command_args");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < command_args_count; j++) {
            (*command_args)[j] = strdup(args[i + j]);
            if ((*command_args)[j] == NULL) {
                perror("Error al duplicar argumento");
                for (int l = 0; l < j; l++) {
                    free((*command_args)[l]);
                }
                free(*command_args);
                exit(EXIT_FAILURE);
            }
        }
        (*command_args)[command_args_count] = NULL;
        if(command_args_count == 0)
        {
        	*command_args = NULL;
        }
    }
}


jobRespawnable *crearJobRespawnable(int *pid_fork, int background, char *args[], int size, int *booleanaFG, char *file_in, char *file_out, int * segundosDelay, int mask_count, int *mask_array) {
    // Asignar memoria para una nueva estructura jobRespawnable
    jobRespawnable *nuevo_jobRes = malloc(sizeof(jobRespawnable));
    if (nuevo_jobRes == NULL) {
        // En caso de error al asignar memoria, imprimir mensaje de error y salir con un cÃ³digo de error
        perror("Error al asignar memoria para jobRespawnable");
        exit(EXIT_FAILURE);
    }

    // Copiar los valores de los parÃ¡metros a la estructura jobRespawnable
    nuevo_jobRes->pid_fork = pid_fork;
    nuevo_jobRes->background = background;
    nuevo_jobRes->segundosDelay = *segundosDelay;
    nuevo_jobRes->segundosDeVida = 0;
    nuevo_jobRes->size = size;
    
    // Copiar los argumentos (args) a la estructura jobRespawnable
    int args_len = 0;
    while (args[args_len] != NULL) {
        args_len++;
    }
    nuevo_jobRes->args = malloc((args_len + 1) * sizeof(char *));
    if (nuevo_jobRes->args == NULL) {
        // En caso de error al asignar memoria, imprimir mensaje de error y salir con un cÃ³digo de error
        perror("Error al asignar memoria para args");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < args_len; i++) {
        nuevo_jobRes->args[i] = strdup(args[i]);
        if (nuevo_jobRes->args[i] == NULL) {
            // En caso de error al duplicar el argumento, imprimir mensaje de error y salir con un cÃ³digo de error
            perror("Error al duplicar el argumento");
            exit(EXIT_FAILURE);
        }
    }
    nuevo_jobRes->args[args_len] = NULL; // Terminar la lista con NULL

    // Copiar file_in y file_out a la estructura jobRespawnable
    if(file_in != NULL)
    {
    	nuevo_jobRes->file_in = strdup(file_in);
    	if (nuevo_jobRes->file_in == NULL) {
        	// En caso de error al duplicar file_in, imprimir mensaje de error y salir con un cÃ³digo de error
        	perror("Error al duplicar file_in");
        	exit(EXIT_FAILURE);
    	}
	
    }
    
    if(file_out != NULL)
    {
    	nuevo_jobRes->file_out = strdup(file_out);
    	if (nuevo_jobRes->file_out == NULL) {
        	// En caso de error al duplicar file_out, imprimir mensaje de error y salir con un cÃ³digo de error
        	perror("Error al duplicar file_out");
        	exit(EXIT_FAILURE);
    	}
    }
     
    nuevo_jobRes->booleanaFG = booleanaFG;	    // Copiar booleanaFG a la estructura jobRespawnable
    
     // Asignar y copiar los valores de mask a la estructura jobRespawnable
    nuevo_jobRes->contadorMask = mask_count;

    nuevo_jobRes->mask = malloc(mask_count * sizeof(int));
    if (nuevo_jobRes->mask == NULL) {
        perror("Error al asignar memoria para mask");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < mask_count; i++) {
        nuevo_jobRes->mask[i] = mask_array[i];
    }

    return nuevo_jobRes; // Devolver el puntero a la nueva estructura jobRespawnable creada
}

void dividir(int * pid_fork, int background, char *args[], int size, int * booleanaFG, char *file_in, char *file_out, int *segundosDeVida, int *segundosDelay, int mask_count, int *mask_array)
{
	jobRespawnable *jobRes = NULL;
	
	// Si el comando se ejecuta en modo RESPAWNABLE, crear una estructura jobRespawnable
	if(background == 2)
	{
		jobRes = crearJobRespawnable(pid_fork, background, args, size, booleanaFG, file_in, file_out, segundosDelay, mask_count, mask_array);
	}
	 
	// Si booleanaFG es 0, significa que el proceso se ejecutarÃ¡ en primer plano
	if(*booleanaFG == 0)
	{
        *pid_fork = fork();		// Crear un nuevo proceso hijo
	}
		
    if (*pid_fork == 0) //Hijo
    {
        proceso_hijo(args, file_in, file_out, mask_count, mask_array);	// Llamar a la funciÃ³n proceso_hijo para ejecutar el comando
    } 
    else if (*pid_fork < 0) //Error con el hijo 
    {
        // Imprimir mensaje de error y salir con un cÃ³digo de error
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else //Padre
    {
        proceso_padre(pid_fork, background, args, booleanaFG, jobRes, segundosDeVida, segundosDelay);	// Llamar a la funciÃ³n proceso_padre para manejar el proceso hijo
    }
}


void *delay_thread(void *arg) {
    jobRespawnable *tarea = (jobRespawnable *)arg;    
    sleep(tarea->segundosDelay); // Esperar los segundos especificado

    printf("Ejecutando comando '%s' despuÃ©s de esperar %d segundos en segundo plano:\n", tarea->args[0], tarea->segundosDelay);
    tarea->background = 1;
	dividir(tarea->pid_fork, tarea->background, tarea->args, tarea->size, tarea->booleanaFG, tarea->file_in, tarea->file_out, &tarea->segundosDeVida, &tarea->segundosDelay, tarea->contadorMask, tarea->mask);
    return NULL;
}

void delay(jobRespawnable *tarea) {
    
	// Crear el hilo
    pthread_t thread;
    if (pthread_create(&thread, NULL, delay_thread, tarea) != 0) {
        perror("Error al crear el hilo");
        free(tarea); // Liberar la memoria de job
        exit(EXIT_FAILURE);
    }
    pthread_detach(thread); // Desvincular el hilo para que se limpie automÃ¡ticamente
}


void manejador(int sig) {
    block_SIGCHLD();
    job *tarea;
    int status;
    enum status status_res;
    int info;

	// Esta funciÃ³n busca en la lista de tareas (listaTareas) el elemento cuyo PID coincida con el PID del proceso hijo
	// que ha cambiado su estado. Si encuentra un elemento que coincida, devuelve un puntero a ese elemento
	// De lo contrario, devuelve NULL.
    while ((tarea = get_item_bypid(listaTareas, waitpid(-1, &status, (WNOHANG | WUNTRACED | WCONTINUED)))) != NULL) 
    {
   
       		status_res = analyze_status(status, &info);		// Analizar el estado del proceso hijo	
        
        if(status_res == EXITED || status_res == SIGNALED)		 // Si el proceso hijo ha terminado o ha sido interrumpido
        {
        	
        	printf("Background job finished: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	if(tarea->state == RESPAWNABLE)		// Si el proceso hijo es RESPAWNABLE, se vuelve a crear
        	{
        		
        		dividir(tarea->respawnable->pid_fork, tarea->respawnable->background, tarea->respawnable->args, tarea->respawnable->size, tarea->respawnable->booleanaFG, tarea->respawnable->file_in, tarea->respawnable->file_out, &tarea->respawnable->segundosDeVida, &tarea->respawnable->segundosDelay, tarea->respawnable->contadorMask, tarea->respawnable->mask);
        	}
        	
        	if(tarea->state != RESPAWNABLE)
        	{
        		liberarJobRespawnable(tarea->respawnable);
        	}
        	
        	delete_job(listaTareas, tarea);		// Eliminar el proceso hijo de la lista de tareas
        	if(status_res == SIGNALED && info == 2)
        	{        		
        		killpg(getpgid(getpid()), SIGTERM);
        	}
        	if(autokill == 1){
        		matados++;	
        	}
        	
        }	
        else if(status_res == SUSPENDED)
        {
        	printf("Background job: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	tarea->state = STOPPED;
        	
        }
        else if(status_res == CONTINUED)
        {
        	
        	printf("Background job: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	tarea->state = BACKGROUND;
        }
        

    }
    unblock_SIGCHLD();
}

void manejadorFor(int sig) {
    block_SIGCHLD();
    job *tarea;
    int status;
    enum status status_res;
    int info;

	// Esta funciÃ³n busca en la lista de tareas (listaTareas) el elemento cuyo PID coincida con el PID del proceso hijo
	// que ha cambiado su estado. Si encuentra un elemento que coincida, devuelve un puntero a ese elemento
	// De lo contrario, devuelve NULL.
    //while ((tarea = get_item_bypid(listaTareas, waitpid(-1, &status, (WNOHANG | WUNTRACED | WCONTINUED)))) != NULL) 
    for(int i = list_size(listaTareas); i>=1; i--)
    {
    	tarea = get_item_bypos(listaTareas, i);
    	
    	int p = waitpid(tarea->pgid, &status, (WNOHANG | WUNTRACED | WCONTINUED));
    
    	if(p == tarea->pgid){
   
       		status_res = analyze_status(status, &info);		// Analizar el estado del proceso hijo	
        
        if(status_res == EXITED || status_res == SIGNALED)		 // Si el proceso hijo ha terminado o ha sido interrumpido
        {
        	
        	printf("Background job finished: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	if(tarea->state == RESPAWNABLE)		// Si el proceso hijo es RESPAWNABLE, se vuelve a crear
        	{
        		
        		dividir(tarea->respawnable->pid_fork, tarea->respawnable->background, tarea->respawnable->args, tarea->respawnable->size, tarea->respawnable->booleanaFG, tarea->respawnable->file_in, tarea->respawnable->file_out, &tarea->respawnable->segundosDeVida, &tarea->respawnable->segundosDelay, tarea->respawnable->contadorMask, tarea->respawnable->mask);
        	}
        	
        	if(tarea->state != RESPAWNABLE)
        	{
        		liberarJobRespawnable(tarea->respawnable);
        	}
        	
        	delete_job(listaTareas, tarea);		// Eliminar el proceso hijo de la lista de tareas
        	if(status_res == SIGNALED && info == 2)
        	{
        		killpg(getpgid(getpid()), SIGTERM);
        	}
        	
        }	
        else if(status_res == SUSPENDED)
        {
        	printf("Background job: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	tarea->state = STOPPED;
        	
        }
        else if(status_res == CONTINUED)
        {
        	
        	printf("Background job: pid: %d, command: %s, %s, info: %d\n", tarea->pgid, tarea->command, status_strings[status_res], info);
        	tarea->state = BACKGROUND;
        }
        }

    }
    unblock_SIGCHLD();
}

void maneSIGUP(int sig){
	FILE *fp;
        fp=fopen("hup.txt","a"); // abre un fichero en modo 'append'
        fprintf(fp, "SIGHUP recibido.\n"); //escribe en el fichero 
        fclose(fp);
}

void maneSIGINT(int sig){
	vecesC++;
	if(vecesC==3){
		exit(127);
	}
}

void maneVacio(int sig){

}

void *autokill_thread(void *arg) {
    sleep(10);
    vecesMatados++;
    manejador(1);
    printf("Limpiados: %d\n", matados);
    if(autokill == 1){
    	autokill_thread(NULL);
    }
    return NULL;
}

void autokiller() {
    
	// Crear el hilo
    pthread_t thread;
    if (pthread_create(&thread, NULL, autokill_thread, NULL) != 0) {
        perror("Error al crear el hilo");
        exit(EXIT_FAILURE);
    }
    pthread_detach(thread); // Desvincular el hilo para que se limpie automÃ¡ticamente
}

void mover_izquierda(char *args[]) {
    int i;
    for (i = 2; args[i] != NULL; i++) {
        args[i - 2] = args[i];
    }
    	args[i - 2] = NULL;  // Terminar la lista de cadenas
	}

int main(void)
{
    char inputBuffer[MAX_LINE];  // Buffer para almacenar el comando ingresado
    int background;              // Indicador de si el comando debe ejecutarse en segundo plano
    char *args[MAX_LINE/2];      // Argumentos del comando
    int pid_fork, pid_wait;      // PIDs del proceso hijo y proceso esperado
    int status;                  // Estado devuelto por waitpid
    enum status status_res;      // Estado procesado por analyze_status
    int info;                    // InformaciÃ³n procesada por analyze_status
    listaTareas = new_list("Lista de tareas");
	int booleanaFG = 0;
	int segundosDeVida = 0;
	int segundosDelay = 0;
	int mask_count = 0;
    int *mask_array = NULL;
		
    ignore_terminal_signals();
    signal(SIGCHLD, manejador);
    signal(SIGHUP, maneSIGUP);
    signal(SIGINT, maneSIGINT);
	
	
    while (1)
    {
        printf("COMMAND->");
        fflush(stdout);
        //printf("\nvecesMatados: %d, matados:%d\n", vecesMatados, matados);
        int size = get_command(inputBuffer, MAX_LINE, args, &background, vecesMatados, matados);
		
		char *file_in, *file_out;
		FILE *entradaEstandar, *salidaEstandar;
		entradaEstandar = stdin;
		salidaEstandar = stdout;
        parse_redirections(args, &file_in, &file_out);

        if (args[0] == NULL)
            continue;
	
		//Compruebo si se usa cd
		if (strcmp(args[0], "cd") == 0)
		{
			comprobar_cd(args);
			continue;
		}
     	
     	//Compruebo si se usa jobs	
     	if(strcmp(args[0], "jobs") == 0)
     	{
     		comprobar_jobs(args);
     		continue;
     	}
     	
     	if(strcmp(args[0], "currjob") == 0)
     	{
     		comprobar_currjob(args);
     		continue;
     	}
		
		
		if(strcmp(args[0], "deljob") == 0)
     	{
     		comprobar_deljob(args);
     		continue;
     	}
			
		if(strcmp(args[0], "zjobs") == 0)
     	{
     		traverse_proc();
     		continue;
     	}	
		
		if(strcmp(args[0], "bgteam") == 0)
     	{
     		if(args[1] != NULL && args[2] != NULL)
			{
				int veces = atoi(args[1]);
				if(veces > 0){
					mover_izquierda(args);
					for(int i=0; i<veces; i++){
						background = 1;
    					dividir(&pid_fork, background, args, size, &booleanaFG, file_in, file_out, &segundosDeVida, &segundosDelay, mask_count, mask_array);
					}
				}
			} else {
				printf("El comando bgteam requiere dos argumentos\n");
			}
     		continue;
     	}
     	
     	if(strcmp(args[0], "parar") == 0)
     	{
     		int paradas = 0;
     		for(int i=1; i<=list_size(listaTareas); i++){
     			job* tarea = get_item_bypos(listaTareas, i);
     			if(tarea->state == 1){
     				paradas++;
     			kill(tarea->pgid, SIGSTOP);
     			printf("Tarea: %s, State: %d\n", tarea->command ,tarea->state);
     			}
     		}
     		printf("Tareas Paradas: %d\n", paradas);
     		continue;
     	}
     	
     	if (strcmp(args[0], "desactivar") == 0) 
		{
			autokill = 1;
			autokiller();
			signal(SIGCHLD, maneVacio);		    
		    printf("Manejador quitado\n");
		    continue;
		}
		
		if (strcmp(args[0], "activar") == 0) 
		{
			autokill = 0;
			signal(SIGCHLD, manejadorFor);
			manejadorFor(1);	    
		    printf("Manejador restaurado\n");
		    continue;
		}
		
		if (strcmp(args[0], "limpiar") == 0) 
		{
			manejador(1);	    
		    printf("Limpiado\n");
		    continue;
		}
			
		//Compriebo si se usa bg
		if (strcmp(args[0], "bg") == 0) 
		{
		    comprobar_bg(args);
		    background = 1;
		    continue;
		}

		//Compruebo si se usa fg
		if (strcmp(args[0], "fg") == 0)
		{
		    comprobar_fg(args, &booleanaFG, &pid_fork);
		    background = 0;
		    if(booleanaFG == 0)
		    {
		    	continue;
		    }
		    
		}
		
		//Comprobar si se usa alarm-thread
		if (strcmp(args[0], "alarm-thread") == 0) 
		{
			if(args[1] != NULL && args[2] != NULL)
			{
				segundosDeVida = atoi(args[1]);
				mover_izquierda(args);
			}
			else
			{
				printf("Los argumentos son incorrectos para ese comando");
				continue;
			}
		}
		
		//Comprobar si se usa delay-thread
		if(strcmp(args[0], "delay-thread") == 0) 
		{
			if(args[1] != NULL && args[2] != NULL)
			{
				segundosDelay = atoi(args[1]);
				mover_izquierda(args);
				jobRespawnable *jobRes = crearJobRespawnable(&pid_fork, background, args, size,  &booleanaFG, file_in, file_out, &segundosDelay, mask_count, mask_array);
				delay(jobRes);
			}
			else
			{
				printf("Los argumentos son incorrectos para ese comando");
			}
			continue;
		}
		
		
		//Comprobar si usa mask
		if(strcmp(args[0], "mask") == 0) 
		{
			
            char **command_args;

            parse_mask_command(args, &mask_count, &mask_array, &command_args);
			if(command_args == NULL)
			{
				printf("Error: la funcion mask estÃ¡ escrita de forma incorrecta\n");
				continue;
			}
			
			jobRespawnable *jobRes = crearJobRespawnable(&pid_fork, background, command_args, size,  &booleanaFG, file_in, file_out, &segundosDelay, mask_count, mask_array);
			dividir(jobRes->pid_fork, jobRes->background, jobRes->args, jobRes->size, jobRes->booleanaFG, jobRes->file_in, jobRes->file_out, &jobRes->segundosDeVida, &jobRes->segundosDelay, jobRes->contadorMask, jobRes->mask);
			continue;
		}
		
		//Hago fork
		dividir(&pid_fork, background, args, size, &booleanaFG, file_in, file_out, &segundosDeVida, &segundosDelay, mask_count, mask_array);
		
    }
    return 0;
}


