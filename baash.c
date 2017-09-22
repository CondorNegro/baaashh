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


#define Color_Red "\x1B[31m"      //Color del hostname y nombre de usuario.
#define Color_Blue "\033[22;34m" // Color del directorio.
#define Color_end "\033[0m"      // Para eliminar configuraciones anteriores.
#define Color_Green "\x1b[32m"
#define LOG_BLUE(X) printf ( "%s %s %s", Color_Blue , X , Color_end ) //Funciones para aplicar el color.
#define LOG_RED(X,Z) printf ( "%s %s@%s %s" , Color_Red , X , Z , Color_end ) 
#define LOG_GREEN(X) printf ( "%s %s %s" , Color_Green , X , Color_end )


/**
 * Se encarga de concatenar dos cadenas de caracteres, sin modificar ninguna de las dos. 
 * @param s1 Puntero a la primer cadena.
 * @param s2 Puntero a la segunda cadena.
 * @return char Cadena producto de la concatencación.
 */
char* concat ( const char *s1, const char *s2) {
	char auxiliar[500]; //Compilador reserva la memoria. (Es una variante a malloc sin necesidad del free).
	strcpy(auxiliar, s1); 
	strcat(auxiliar,s2);
    char *resultado;
    resultado=auxiliar;
    return resultado;
}

/**
 * Se encarga de parsear el comando ingresado como un conjunto de argumentos. 
 * @param argv Arreglo donde se colocarán los argumentos hallados.
 * @param cadena Comando ingresado.
 * @return int Numero de argumentos.
 */
int parse_Command( char* argv[] , char* cadena ) {
	int argscounter = 0;
	argv[0] = strtok( cadena , " \n" );
	for ( argscounter = 1 ; argscounter < 20 ; argscounter++ ) {
		argv[argscounter] = strtok ( NULL , " \n" ); //Para la separación se usa el salto de línea.
		if ( argv[argscounter] == NULL )
			break;
	}
	return argscounter; //Cantidad de argumentos. (argc).
}


/**
 * Se encarga de obtener el arreglo de Paths.
 * @param paths Arreglo donde se colocarán las direcciones que se encuentran en la variable de entorno PATH.

 * @return int Numero de Paths.
 */
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

/**
 * Se encarga de encontrar la ruta para ejecutar el comando, de acuerdo a si se otorgó PATH relativo o absoluto. 
 * @param paths Arreglo de punteros a las direcciones contenidas en el PATH.
 * @param cantidad Cantidad de direcciones que componen el PATH.
 * @param comando Puntero a la cadena del comando.
 * @param clasificacion Puntero a la cadena de la clasificacion del comando.
 * @param actualdir Directorio actual de trabajo (pwd).
 * @return char Ruta de ejecucion del comando.
 */
char * getRutaEjecucion ( char* paths[] , int cantidad , char* comando , char* clasificacion, char* actualdir ) {
	if ( strcmp(clasificacion,"comando") == 0 ) {
		for ( int i = 0 ; i < cantidad ; i++ ) {
			if ( access ( concat ( concat ( paths[i] , "/" ) , comando ) , X_OK ) == 0 ) {
			return concat ( concat ( paths[i] , "/" ) , comando );
			}
		}
	}
	else if ( strcmp(clasificacion,"absoluto") == 0 ){
		if ( access( comando , X_OK ) == 0 ) { //Access intenta ejecutar el comando gracias al parámetro X_OK.
			return comando;
		}
	}
	else if ( strcmp(clasificacion,"relativo") == 0 || strcmp(clasificacion,"relativo-r") == 0 ){
		if ( access ( comando , X_OK ) == 0 ) {
			return comando;
		}
	}
	
	if( errno != 0 ) { //No encontró la ruta de ejecucion.
		fprintf ( stderr , "getRutaEjecucion ERRNO: %d\n" , errno );
    	perror ( "Error" );
    	return "[error]";
	}
}


/**
 * Se encarga de clasificar el comando en absoluto (/) , relativo (./) , relativo-r (../) o comando.
 * @param comando Es el comando a clasificar.

 * @return char* Puntero al string de clasificacion.
 */
char * getClasificacionComando ( char* comando ) {  // Clasifica el comando en absoluto , relativo , relativo-r o comando.
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


/**
 * Se encarga de ejecutar el archivo pasado como parametro, junto con sus argumentos.
 * @param ruta Es la ruta del archivo a ejecutar.

 * @return void
 */
void ejecutarArchivo (  char* ruta , char* argumentos[] ) {
	
	execv( ruta , argumentos ); //Utilización de execv.
	if( errno != 0 ) {  		//Error en caso de no poder ejecutarse el archivo.
		fprintf ( stderr , "ejecutarArchivo ERRNO: %d\n" , errno );
    	perror ( "Error" );
    	exit(EXIT_FAILURE);
	}


}


/**
 * Gestor de operaciones del comando interno cd.
 * @param argumento Puntero al directorio al cual se quiere ir.

 * @return void
 */
void cdBuiltin ( char* argumento ) {
	int changeDir;
    changeDir=chdir(argumento); //Cambio de directorio.
	if( changeDir != 0 ) { //Manejo de errores del cambio de directorio.
		fprintf ( stderr , "ERRNO: %d\n" , errno );
    	perror ( "Error" );
	}
	
}

/**
 * Remplaza el /home/username por el caracter ~.
 * @param directorioDeTrabajo Arreglo que contiene el directorio actual de trabajo.
 
 * @return Devuelve el directorio a imprimir por consola.
 */

char *acondicionarHome(char directorioDeTrabajo[]){
	char *dir=directorioDeTrabajo;
	char *pointer;
	char *pointer2;
	int barra=(int)'/';


	pointer=strstr(dir , "/home/"); //Detecta presencia de /home/ en el directorio.
	int contador=0;
	if ( pointer != NULL ){
		pointer2=strchr(dir,barra);
		pointer2=strchr(pointer2+1,barra);
		
		if (pointer2!=NULL){
			pointer2=strchr(pointer2+1,barra); //Detecta la cantidad de barras "/".
			char *enie="~";
			if(pointer2!=NULL){
				char *pointer3=concat(enie,pointer2); //Reemplazo.
				return pointer3;
			}
			else{
				char *pointer3=concat(enie," $"); //Caso especial: ~ $
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
	int cantidadpaths= getPaths ( paths );
	char fileName[100];
	int flagRedirect = 0;
	int flagPlano=0;
	
	int flagSaltoLinea=0;
	int contadorImpresion=0;
	int volatile contadorHijos=0; //Para saber el numero de hijos.
	int volatile contadorHijosAnterior=-1; //Por estetica de la consola. Mostrar ejecución de hilo anterior.
	
   
    while ( ! feof ( stdin ) ) { 		//Ejecución siempre y cuando no aparezca un Ctrl+D.
    	
	    getcwd(actualdir, 100);        //Directorio de trabajo almacenado en actualdir.
		LOG_RED(getlogin(), hostname); //Imprime el uid/gid
		strcat(actualdir, " $");
		char *dir=acondicionarHome(actualdir);
		LOG_BLUE(dir);
		
		
	  	strcpy(buffer, "\n"); 		 
    	fgets(buffer,1024,stdin);
    	if (strcmp(buffer,"\n")==0 ) { //Si se ingresa '\n'.
    		continue;
    	}

    	else{
    		argc=parse_Command(argv,buffer);
    		if( strcmp (argv[0] , "cd") == 0 && argc>2){ // Comando interno cd con directorios que presentan 
    													// espacios en blanco.
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
    		if ( strcmp ( argv[0] , "cd" ) == 0 && argc > 1 ) { // Comando interno cd con directorios que no presentan 
    															// espacios en blanco.
    			cdBuiltin ( argv[1] );
    			
    			continue;
    		}
     		if ( (strcmp ( argv[0] , "cd" ) == 0 && argc == 1) || strcmp(argv[0] , "~") == 0 ) { //Ingreso de ~.
     																							//Ingreso de cd (solamente).
    			cdBuiltin(getenv("HOME"));
    			
    			continue;
    		}

    		if ( strcmp ( argv[0] , "exit" ) == 0 ) { //Ingreso de "exit".

    			return 0;
    		}
    		if (strcmp(argv[0],"..") == 0 ){  //Ingreso de .. (Basado en Shell de Linux).
    			printf("%s\n", "..: No se encontro la orden");
    			continue;
    		}
    		if (strcmp(argv[0],"&") == 0 ){  //Ingreso de .. (Basado en Shell de Linux).
    			printf("%s\n", "bash : error sintactico cerca del elemento inesperado \"&\"");
    			continue;
    		}
    		clasificacion=getClasificacionComando( argv[0] );
    		
    		if(strcmp(argv[argc-1],"&") != 0){
    				flagPlano=1;
    		}
    		
    		
    		else{
    			flagPlano=2;
    			argv[argc-1]=NULL; //Para evitar errores de argumentos en los programas
    								//debido al ampersand.
    			argc--;
    		}
    		ruta=getRutaEjecucion(paths,cantidadpaths,argv[0],clasificacion,actualdir);
    		
    		if (strcmp(ruta,"[error]") != 0 ){
    			contadorHijosAnterior=contadorHijos;
    			contadorHijos++;
    			
    			

    			flagRedirect = checkRedirect(argv, fileName);
    			
    			pid = fork();
    			 			

    			if (pid == 0) { //Proceso hijo.
    				
    				
    				
    				if(flagPlano==1){ //No es en segundo plano la ejecución.
    					
    					if(flagRedirect == 2){
							outPut(fileName);
						}
						else if(flagRedirect == 1){						
							freopen(fileName,"r",stdin);
						}
    					ejecutarArchivo( ruta , argv );
    					
    				}
    				else{
    				
    					
    					if(flagRedirect == 2){
							outPut(fileName);
						}
						else if(flagRedirect == 1){						
							freopen(fileName,"r",stdin);
						}
    					ejecutarArchivo( ruta , argv );
    					
    					
    				}
    				
    				exit(0);
    				
    			}
    			else{
    				if(flagPlano==1){ //Ejecucion de proceso hijo es en primer plano.
    					wait(0); //Wait.
    					sleep(1);
					
    						
						
    					contadorHijos=0;    					
    				}
    				else{ //Ejecucion de proceso hijo es en segundo plano. No espera.
    					
    					printf ("[%d]   %d\n", contadorHijos , pid );  
    					
	    					      					
    				}		
    			}	
    		}

    	}
    	
	    	
    }
    printf("\n");
	return 0;
}
