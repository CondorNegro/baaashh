#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>


#define Color_Red "\x1B[31m"
#define Color_Blue "\033[22;34m" // Color Start
#define Color_end "\033[0m" // To flush out prev settings
#define Color_Green "\x1b[32m"
#define LOG_BLUE(X) printf ( "%s %s %s", Color_Blue , X , Color_end )
#define LOG_RED(X,Z) printf ( "%s %s@%s %s" , Color_Red , X , Z , Color_end )
#define LOG_GREEN(X) printf ( "%s %s %s" , Color_Green , X , Color_end )

char* concat ( const char *s1, const char *s2) {
	char auxiliar[500];
	strcpy(auxiliar, s1);
	strcat(auxiliar,s2);
    char *resultado;
    resultado=auxiliar;
    return resultado;
}

int parseArguments ( char* argv[] , char* cadena ) {
	int argscounter = 0;
	argv[0] = strtok( cadena , " \n" );
	for ( argscounter = 1 ; argscounter < 20 ; argscounter++ ) {
		argv[argscounter] = strtok ( NULL , " \n" );
		if ( argv[argscounter] == NULL )
			break;
	}
	return argscounter;
}

int getPaths ( char* paths[] ) {
	int pathcounter = 0;
	char *pathsaux[25];
	pathsaux[0] = strtok( getenv( "PATH" ) , ":" );
	if( errno != 0 ) {
		fprintf ( stderr , "ERRNO: %d\n" , errno );
    	perror ( "Error" );
    	exit(EXIT_FAILURE);
	}
	for ( pathcounter = 1 ; pathcounter < 20 ; pathcounter++ ) {
		pathsaux[pathcounter] = strtok(NULL, ":");
		if ( pathsaux[pathcounter] == NULL )
			break;
	}
	for ( int i = 0 ; i < pathcounter ; i++ ) {
		paths[i] = pathsaux[i];
	}
	return pathcounter;
}



char * getRutaEjecucion ( char* paths[] , int cantidad , char* comando , char* clasificacion, char* actualdir ) {
	if ( strcmp(clasificacion,"comando") == 0 ) {
		for ( int i = 0 ; i < cantidad ; i++ ) {
			if ( access ( concat ( concat ( paths[i] , "/" ) , comando ) , X_OK ) == 0 ) {
			return concat ( concat ( paths[i] , "/" ) , comando );
			}
		}
	}
	else if ( strcmp(clasificacion,"absoluto") == 0 ){
		if ( access( comando , X_OK ) == 0 ) {
			return comando;
		}
	}
	else if ( strcmp(clasificacion,"relativo") == 0 || strcmp(clasificacion,"relativo-r") == 0 ){
		if ( access ( comando , X_OK ) == 0 ) {
			return comando;
		}
	}
	
	if( errno != 0 ) {
		fprintf ( stderr , "getRutaEjecucion ERRNO: %d\n" , errno );
    	perror ( "Error" );
    	return "[error]";
	}
}

char * getClasificacionComando ( char* comando ) {       // Clasifica el comando en absoluto , relativo , relativo-r o comando
	char * clasificacion;
	if ( strncmp ( comando , "/" , 1 ) == 0 )
		clasificacion="absoluto";
	else if ( strncmp( comando , "./" , 2 ) == 0 )
		clasificacion="relativo";
	else if ( strncmp ( comando , "../" , 3 ) == 0 )
		clasificacion="relativo-r";
	else
		clasificacion="comando";
	return clasificacion;
}

void ejecutarArchivo (  char* ruta , char* argumentos[] ) {
	execv( ruta , argumentos );
	if( errno != 0 ) {
		fprintf ( stderr , "ejecutarArchivo ERRNO: %d\n" , errno );
    	perror ( "Error" );
    	exit(EXIT_FAILURE);
	}
}

void cdBuiltin ( char* argumento ) {
	int changeDir;
    changeDir=chdir(argumento);
	if( changeDir != 0 ) {
		fprintf ( stderr , "ERRNO: %d\n" , errno );
    	perror ( "Error" );
	}
}


char *acondicionarHome(char directorioDeTrabajo[]){
	char *dir=directorioDeTrabajo;
	char *pointer;
	char *pointer2;
	int barra=(int)'/';

	pointer=strstr(dir , "/home/");
	int contador=0;
	if ( pointer != NULL ){
		pointer2=strchr(dir,barra);
		pointer2=strchr(pointer2+1,barra);
		
		if (pointer2!=NULL){
			pointer2=strchr(pointer2+1,barra);
			char *enie="~";
			if(pointer2!=NULL){
				char *pointer3=concat(enie,pointer2);
				return pointer3;
			}
			else{
				char *pointer3=concat(enie," $");
				return pointer3;
				
			}
			
		}
		else{
			return dir;
		}
	}
	else{
		return dir;
	}

}

/**
 * Verifica si se debe redireccionar la entrada o la salida estandar.
 * @param argv Arreglo que contiene el comando y los argumentos ingresados.
 * @param fileName Almacena el nombre del archivo del que se lee, o en el que se escribe la salida.
 * @return Devuelve 0 si no hay que redireccionar, 1 si hay que redireccionar la entrada, y 2 si hay que redireccionar la salida.
 */

int checkRedirect(char* argv[], char fileName[]){
	int i;
	for (i = 0; i < 100; i++){
		char *token;     // para aceptar tanto ">archivo" como "> archivo"
		token=strtok( argv[i] , ">" );
		if(argv[i] == NULL){
			fileName = NULL;
			return 0;
		}
		else if (!strcmp(argv[i], "<")){
			strcpy(fileName, argv[i+1]);
			argv[i] = NULL;		
			argv[i+1] = NULL;
			return 1;
		}
		else if (!strcmp(argv[i], ">")){
			strcpy(fileName, argv[i+1]);
			argv[i] = NULL;	
			argv[i+1] = NULL;
			return 2;
		}
		
		else if (!strncmp(argv[i], ">",1))
		{
			strcpy(fileName, token);
			argv[i] = NULL;	

			return 2;
		}
		else if (!strncmp(argv[i], "<",1))
		{
			strcpy(fileName, token);
			argv[i] = NULL;	

			return 1;
		}
	}
	return 0;
}

/**
 * Modifica la entrada estándar.
 * @param fileName nombre del archivo de entrada.
 */
void inPut(char fileName[]){
	int fid;
	int flags,perm;
	flags = O_RDONLY;
	perm = S_IWUSR|S_IRUSR;
	
	close(STDIN_FILENO);
	fid = open(fileName, flags, perm);	
	if (fid<0) {
		perror("open");
		exit(1);
	}	
	if (dup(fid)<0) {
		perror("dup");
		exit(1);
	}
	close(fid);	
}

/**
 * Modifica la salida estándar.
 * @param fileName nombre del archivo donde se envía la salida.
 */
void outPut(char fileName[]){
	int fid;
	int flags,perm;
	flags = O_WRONLY|O_CREAT|O_TRUNC;
	perm = S_IWUSR|S_IRUSR;
	
	fid = open(fileName, flags, perm);	
	if (fid<0) {
		perror("open");
		exit(1);
	}
	close(STDOUT_FILENO);
	if (dup(fid)<0) {
		perror("dup");
		exit(1);
	}
	close(fid);
}


extern int errno ;	
int main () {
	int argc;
	pid_t pid;
	char *argv[20];
	char *paths[25];
	char buffer[1024];
	char hostname [30];
	gethostname ( hostname , 30 );
	char actualdir[100];
	char * clasificacion;
	char * ruta;
	int cantidadpaths=getPaths ( paths );
	char fileName[100];
	int flagRedirect = 0;
	int volatile contadorHijos=0;
	int volatile contadorHijosAnterior=-1;
	
    while ( ! feof ( stdin ) ) {
    	getcwd(actualdir, 100);
    	LOG_RED(getlogin(), hostname); //imprimo el uid/gid
    	strcat(actualdir, " $");
    	char *dir=acondicionarHome(actualdir);

    	LOG_BLUE(dir);
    	strcpy(buffer, "\n");
    	fgets(buffer,1024,stdin);
    	if (strcmp(buffer,"\n")==0) {
    		continue;
    	}

    	else{
    		argc=parseArguments(argv,buffer);
    		if( strcmp (argv[0] , "cd") == 0 && argc>2){
    			char carpeta[100]="";
    			for (int i = 1; i < argc; ++i){
    				strcat(carpeta , argv[i]);
    				if(argc!=i+1){
    					strcat(carpeta," ");
    				}
    			}
    			cdBuiltin(carpeta);
    			continue;
    		}
    		if ( strcmp ( argv[0] , "cd" ) == 0 && argc > 1 ) {
    			cdBuiltin ( argv[1] );
    			continue;
    		}
     		if ( (strcmp ( argv[0] , "cd" ) == 0 && argc == 1) || strcmp(argv[0] , "~") == 0 ) {
    			cdBuiltin(getenv("HOME"));
    			continue;
    		}

    		if ( strcmp ( argv[0] , "exit" ) == 0 ) {
    			return 0;
    		}
    		if (strcmp(argv[0],"..") == 0 ){
    			printf("%s\n", "..: No se encontro la orden");
    			continue;
    		}
    		clasificacion=getClasificacionComando( argv[0] );
    		ruta=getRutaEjecucion(paths,cantidadpaths,argv[0],clasificacion,actualdir);
    		flagRedirect = checkRedirect(argv, fileName);
    		if (strcmp(ruta,"[error]") != 0 ){
    			contadorHijosAnterior=contadorHijos;
    			contadorHijos++;
    			
    			pid = fork();

    			if (pid == 0) {
    				if(flagRedirect == 2){
						outPut(fileName);
					}
					else if(flagRedirect == 1){						
						freopen(fileName,"r",stdin);
					}
    				
    				if(strcmp(argv[argc-1],"&")!=0){
    					ejecutarArchivo( ruta , argv );

    					
    				}
    				else{
    					
    					char *argvAux[argc];
    					for (int i = 0; i < argc-1; ++i){
    						argvAux[i]=argv[i];
    						
    					}
    					ejecutarArchivo( ruta , argvAux );

    					
    				}
    				
    				exit(0);
    				
    			}
    			else{
    				if(strcmp(argv[argc-1],"&")!=0){
    					
    					wait(0);
    					contadorHijos=0;
    					
    					
    				}
    				else{
    					printf ("[%d]   %d\n", contadorHijos , pid );
    					    					
    				}
    					
    			}
    				
    		}

    	}		
    }
    printf("\n");
	return 0;
}
