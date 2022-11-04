/*
 *Víctor Jorge Sibaja
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define INTERACTIVO 0
#define NO_INTERACTIVO 1
#define MAX_ARGS 128

char error_message[30]= "An error has occurred\n";
int modo;
int redir; //Marca si se ha detectado un '>'

void recogerComandos(FILE *f);
void separado(char *comando);
int builtInCommands(char *miArgv[], int miArgc);
void creaProceso(char *miArgv[], int argc, int marcador);

int main(int argc, char *args[]){

    //En caso de haber más de dos argumentos no es ni modo interactivo ni modo batch
    if (argc > 2){
	fprintf(stderr, "%s", error_message);
	exit(1);
    }
    
    FILE *fd;
    //Corresponde a modo interactivo
    if (argc < 2){
	fd = stdin;
	modo = INTERACTIVO;
    }

    //En caso de no ser modo interactivo tomar fichero para los comandos
    else{
	fd = fopen(args[1],"r");
	modo = NO_INTERACTIVO;
	if (fd == NULL){
	    fprintf(stderr, "%s", error_message);
	    exit(1);
	}
 
    }
    
    recogerComandos(fd);

    exit(0); 
    
}

/*
 * Recopila linea por linea los comandos introducidos del modo interactivo o no interactivo
 */
void recogerComandos (FILE *f){

    char *comando = NULL;
    size_t valor = 0;
    ssize_t tam = 0;
   
    if (modo == INTERACTIVO){
    printf("Uvash>");
    }

    while((tam = getline(&comando, &valor, f)) != -1){  
        
	//Quitar fin de linea
	if (tam > 0){
	    comando[tam-1] = '\0';
	}
    
    
	//Getline se detiene (Introducir Ctrl+D corresponde a EOF --> getline se detiene)
        else{
	    printf("\n");
	    exit(0);
        }
    
	separado(comando);
	if (modo == INTERACTIVO){
	    printf("Uvash>");
	}

    }

    free(comando);
    comando = NULL;
    valor = 0;
}

/*
 * Método que separa los argumentos del comando pasado y se asegura de que la sintaxis es 
 * correcta
 */
void separado(char *comando){
    
    char *comandoT = comando;
    char *delimitador = " ";
    int i = 0;
    char **miArgv; 

    miArgv = (char **) malloc(MAX_ARGS*sizeof(char*));
    
    char *palabra = strtok(comandoT, delimitador);
    miArgv[i] = palabra;
    if (palabra != NULL){
	while(palabra != NULL){
	    palabra = strtok(NULL, delimitador);
	    i ++;
	    miArgv[i] = palabra;
	}
    }
    
    //Evito generar una segV más tarde al usar strstr con un puntero nulo
    else{
	miArgv[i] = "saltolinea";
    }
    
    i++;
    miArgv[i] = NULL;
    
    int marcador = 0;
    redir = FALSE; 
    for (int j = 0; miArgv[j] != NULL; j++){

	//Si previamente ya se había detectado un '>' es un error		
	if ((strstr(miArgv[j],">") != NULL) && redir == TRUE && strcmp(miArgv[j],">") == 0){
	    fprintf(stderr, "%s", error_message);
	    exit(0);
	}

	if (strstr(miArgv[j],">") != NULL && strcmp(miArgv[j],">") == 0){
	    redir = TRUE;
	    marcador = j;
	}
    
    }
    

    /* Casos inválidos de redireccionamiento: más de un argumento, sin argumentos después de >
    o > al principio de la cadena */
    if (redir == TRUE){
	if (miArgv[marcador+1] == NULL || miArgv[marcador+2] != NULL || marcador == 0){
	    fprintf(stderr, "%s", error_message);
	    exit(0);
	}
    }
    
    
    int seguir = builtInCommands(miArgv,i); 

    
    if (seguir == TRUE){
    creaProceso(miArgv,i, marcador);
    }
    
    
    free(miArgv);
}

/*
 * Función que ejecuta los builtInCommands en caso de que el introducido se trate de uno de 
 * ellos
 */
int builtInCommands(char *miArgv[], int miArgc){
    char cadena[5] = "exit";
    char cadena2[3] = "cd";

    int seguir = TRUE;
    
    /* Compruebo que "exit" esté dentro del comando y además que sea exactamente esa palabra
    asegurando que tengan la misma longitud */
    if (strstr(miArgv[0],cadena) != NULL && strcmp(miArgv[0],cadena) == 0){
	if (miArgc > 2){
	    fprintf(stderr, "%s", error_message);
	    exit(0);
	}
	seguir = FALSE;
	exit(0);
    }

    if (strstr(miArgv[0],cadena2) != NULL && strcmp(miArgv[0],cadena2) == 0){
	if (miArgc > 3 || miArgc<3){
	    fprintf(stderr, "%s", error_message);
	    exit(0);
	}

	else{
	    chdir(miArgv[1]);
	    seguir = FALSE;
	}
        
    }

    return seguir;
}

/*
 * Método que crea el proceso para ejecutar los comandos que no sean builtInCommands
 */
void creaProceso(char *miArgv[], int miArgc, int marcador){
    
	//En caso de no haber un salto de linea, el comando se podrá ejecutar de forma normal 
	if (strstr(miArgv[0],"saltolinea") == NULL && strcmp(miArgv[0],"saltolinea") != 0){
	    pid_t pidHijo;
	    int resultado;
    
	    pidHijo = fork();
	    if (pidHijo == 0){
		if (redir == TRUE){
		    /*En caso de haber un redireccionamiento, no se quiere que el comando 
		    *que lance execvp tome como argumento > o lo que haya despues */		    char **nuevoArgv; 
		    nuevoArgv = (char **) malloc(MAX_ARGS*sizeof(char*));

		    for (int i = 0; i<marcador; i++){
			nuevoArgv[i] = miArgv[i];
		    }
		    nuevoArgv[marcador] = NULL;

		    int fout = open(miArgv[marcador+1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
		    dup2(fout,1);
		    dup2(fout,2);
		    close(fout);
		    redir = FALSE;
		    resultado = execvp(nuevoArgv[0], nuevoArgv);
		
		    free(nuevoArgv);	
		} 
		else{	
		    resultado = execvp(miArgv[0], miArgv);
		}
		if (resultado == -1){
		    fprintf(stderr, "%s", error_message);
		    exit(1);
		}   

	}
	wait(NULL); 
    }
}
 
