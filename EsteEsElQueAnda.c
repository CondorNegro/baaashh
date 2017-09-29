#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

//Funciones presentes.
void getPaths(char* paths []);
int leerTeclado(char* argv[], char* cadena);
void buscarArchivo(char* arch, char* paths[], char* execPath);
int background(char* argv[], int argc);
int checkRedirect(char* argv[], char fileName[]);
int checkPipe(char* argv[], char* argv1[], char* argv2[]);
void outPut(char fileName[]);
void inPut(char fileName[]);
void doPipeline(char* argv1[], char* argv2[], char* paths[]);
char* concat ( const char *s1, const char *s2);


#define Color_Red "\x1B[31m"      //Color del hostname y nombre de usuario.
#define Color_Blue "\033[22;34m" // Color del directorio.
#define Color_end "\033[0m"      // Para eliminar configuraciones anteriores.
#define Color_Green "\x1b[32m"
#define LOG_BLUE(X) printf ( "%s %s %s", Color_Blue , X , Color_end ) //Funciones para aplicar el color.
#define LOG_RED(X,Z) printf ( "%s %s@%s %s" , Color_Red , X , Z , Color_end ) 
#define LOG_GREEN(X) printf ( "%s %s %s" , Color_Green , X , Color_end )





/**
* Obtiene los Paths de la variable de entorno PATH, y almacena uno por cada elemento del array paths.
* @param paths Arreglo de punteros a caracter donde se almacena cada path.
*/
void getPaths( char* paths[] ) {
	int pathCounter;
	char* pathVar = getenv("PATH");
	paths[0] = strtok(pathVar, ":");
	for( pathCounter = 1 ; pathCounter < 20 ; pathCounter++ ) {
		paths[pathCounter] = strtok(NULL,":");    
		if ( paths[pathCounter] == NULL ) {
			break;
		}
	}
	strtok ( NULL , ":" );
}


/**
 * Se encarga de parsear el comando ingresado como un conjunto de argumentos. 
 * @param argv Arreglo donde se colocarán los argumentos hallados.
 * @param cadena Comando ingresado.
 * @return int Numero de argumentos.
 */
int parse_Command ( char* argv[] , char* cadena ) {
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
* Gestor de operaciones del comando interno cd.
* @param argumento Puntero al directorio al cual se quiere ir.
* @return void
*/
void cdBuiltin ( char* argumento ) {
	int changeDir;
	changeDir = chdir ( argumento ); //Cambio de directorio.
	if( changeDir != 0 ) { //Manejo de errores del cambio de directorio.
		
		perror ( "Error" );
	}

}

/**
* Lee la cadena almacenada en arch, busca el ejecutable, y muestra el PATH donde se encuentra
* @param arch Es el path donde se encuentra el archivo o directorio que se quiere buscar
* @param paths Arreglo con todos los directorios en los que se quiere buscar
* @param execPath Es el path completo al ejecutable que se encontro. Si no se encuentra el archivo, se pone 'X' en execPath[0]
*/


void buscarArchivo ( char* arch , char* paths[] , char* execPath ) {
	
	int result;
	char searchDir[150] = "";
	char* archivo;
	
	int flagPuntosBarras=1;
	int flagTresPuntos=0;
	int contTresPuntos=0;
	int indiceAnteriorTresPuntos=0;
	for (int i = 0; i < strlen(arch); i++){
			
				if((arch[i])=='.'){	
							if(i-indiceAnteriorTresPuntos==1){
								contTresPuntos++;
			
							}
							indiceAnteriorTresPuntos=i;
							
							if(contTresPuntos==2){
								contTresPuntos=0;
								flagTresPuntos=1;
							}
						}
						else{
							indiceAnteriorTresPuntos=0;
							contTresPuntos=0;
						}
						if(arch[i]!='/' && arch[i]!='.'){
							flagPuntosBarras=0;
						}
			
	}
	
	if(flagPuntosBarras && !flagTresPuntos){
		execPath[0] = 'D';
		return;
	}
	if(flagTresPuntos){
		execPath[0] = 'X';
		return;
	}
	if ( arch[0] == '/' || ( arch[0] == '.' && arch[1] == '.' && arch[2] == '/' ) ) {
		char* dir;
		char* nextDir;
		int pathReady = 0; //Bandera que controla si ya se obtuvo el path completo
		if ( arch[0] == '/' )
			searchDir[0] = '/';
		dir = strtok ( arch , "/" );
		nextDir = strtok ( NULL , "/" );
		if ( nextDir != NULL ) //Si el comando no es de la forma /archivo
			strcat( searchDir , dir );
		else {
			nextDir = dir; //Si archivo si es NULL, entonces pongo en archivo lo que hay despues de la ultima /
			pathReady = 1;
		}
		while ( (nextDir != NULL) && !pathReady ) {
			dir = nextDir;
			nextDir = strtok ( NULL , "/" );
			strcat ( searchDir , "/" );
			if ( nextDir != NULL )
				strcat ( searchDir , dir );
		}
		archivo = dir;
	}
	
	else if ( arch[0] == '.' && arch[1] == '/' ) { //Es un path relativo, tomando como path el directorio actual.
		getcwd ( searchDir , 150 );
		if(searchDir==NULL){
			perror("Error en obtener directorio actual.");
			exit(1);
		}
		else{
			strcat ( searchDir , "/" );
			const char ch = '/';
   			archivo = strchr(arch, ch);
   		}

	}

	else { //Tiene que buscar en todos los directorios del path.
		int i;
		char aux[150];
		for ( i = 0 ; i < 20 ; i++ ) {
			if(paths[i] == NULL)
				break;
			strcpy ( aux , paths[i] );
			strcat ( aux , "/" );
			strcat ( aux , arch );
			result = access ( aux , X_OK );

			if ( !result ) {
				strcpy ( execPath , aux );
				return;
			}

		}
		execPath[0] = 'X';
		return;
	}

	strcat ( searchDir , archivo );

	result = access ( searchDir , X_OK );
	if( !result ){
		strcpy ( execPath , searchDir );
		execPath[0] = 'D';
		
		return;
	}
	else
		execPath[0] = 'X';

	
	
}


/**
* Verifica si se quiere lanzar el proceso en background.
* @param argv Vector de argumentos que pasa el usuario. Se necesita para verificar si el ultimo es un &.
* @return Devuelve 0 si el proceso no debe ejecutarse en background, y 1 en caso de hacerlo porque el ultimo argumento es & y 2 si es argumento&.
*/
int background ( char* argv[] , int argc ) {
	int i;
	const char ch = '&'; //Para caso especial por ejemplo:  'ls&'.
	char *resto;
	resto = strrchr(argv[argc-1], ch);
	for ( i = 0 ; i < 20 ; i++ ) {
		if ( argv[i] == NULL )
			break;
	}
	if( !strcmp ( argv[i-1] , "&" ) ) {
		return 1;
	}

	if ( resto!=NULL ) {
		char *token;   
		token = strtok( argv[argc-1] , "&\n" );
		argv[argc-1] = token;
		return 2;
	}
	return 0;
}

/**
* Verifica si se debe redireccionar la entrada o la salida estandar.
* @param argv Arreglo que contiene el comando y los argumentos ingresados.
* @param fileName Almacena el nombre del archivo del que se lee, o en el que se escribe la salida.
* @return Devuelve 0 si no hay que redireccionar, 1 si hay que redireccionar la entrada, y 2 si hay que redireccionar la salida.
*/
int checkRedirect ( char* argv[] , char fileName[] ) {
	int i;
	for ( i = 0 ; i < 20 ; i++ ) {
		if ( argv[i] == NULL ) {
			fileName = NULL;
			return 0;
		}
		else if ( !strcmp ( argv[i] , "<" ) ) {
			strcpy ( fileName , argv[i+1] );
			argv[i] = NULL;   
			argv[i+1] = NULL;
			return 1;
		}
		else if (!strcmp(argv[i], ">")){
			strcpy ( fileName , argv[i+1] );
			argv[i] = NULL; 
			argv[i+1] = NULL;
			return 2;
		}
	}
	return 0;
}


/**
* Modifica la salida estándar.
* @param fileName Path al archivo donde se envía la salida.
*/
void outPut ( char fileName[] ) {
	int fid;
	int flags,perm;
	flags = O_WRONLY|O_CREAT|O_TRUNC;
	perm = S_IWUSR|S_IRUSR;
	fid = open ( fileName , flags , perm );  
	if ( fid <0 ) {
		perror ( "Error en apertura de archivo." );
		exit ( 1 );
	}
	int valor=close( STDOUT_FILENO );
	if(valor<0){
		perror("Error en cierre de archivo.");
		exit(1);
	}
	if ( dup ( fid ) < 0 ) {
		perror("Error en duplicacion de ficheros.");
		exit(1);
	}
	valor=close(fid);
	if(valor<0){
		perror("Error en cierre de archivo.");
		exit(1);
	}

}

/**
* Modifica la entrada estándar.
* @param fileName Path al archivo donde se envía la salida.
*/
void inPut ( char fileName[] ) {
	int fid;
	int flags,perm;
	flags = O_RDONLY;
	perm = S_IWUSR|S_IRUSR;
	int valor=close ( STDIN_FILENO );  //cierro entrada estandar
	if(valor<0){
		perror("Error en cierre de archivo.");
		exit(1);
	}
	fid = open ( fileName , flags , perm );  //trato de abrir archivo de entrada secundaria fileName
	if ( fid < 0 ) {
		perror ( "Error intentando abrir el archivo de entrada." );
		exit ( 1 );
	} 
	if ( dup ( fid ) < 0 ) {
		perror ( "Error en duplicacion de ficheros." );
		exit ( 1 );
	}
	valor=close ( fid );  //cierro el archivo
	if(valor<0){
		perror("Error en cierre de archivo.");
		exit(1);
	}
}


/**
* Verifica si debe realizarse un pipeline.
* @param argv Argumentos del comando ingresado por el usuario.
* @param argv1 Array donde se guardarán los argumentos del comando 1.
* @param argv2 Array donde se guardarán los argumentos del comando 2.
* @return Devuelve 1 si se debe ejecutar el pipeline. 0 en caso contrario.
*/
int checkPipe ( char* argv[] , char* argv1[] , char* argv2[] ) {
	int indexArgumento, indexArgumento2;
	for( indexArgumento = 0 ; argv[indexArgumento] != NULL ; indexArgumento++ ) {
		int aux = strcmp ( "|" , argv[indexArgumento] );
		if( aux == 0 )
			break;
		argv1[indexArgumento] = argv[indexArgumento];
	}
	argv1[indexArgumento] = '\0';
	//Si no encontró |, se devuelve un 0, si no, se obtienen los dos argv de los comandos.
	if ( argv[indexArgumento] == NULL )
		return 0;
	indexArgumento++; //aumento para saltear el | 
	for ( indexArgumento2 = 0 ; argv[indexArgumento] != NULL ; indexArgumento2++ ) {
		if ( argv[indexArgumento2+indexArgumento] == NULL )
			break;
		argv2[indexArgumento2] =  argv[indexArgumento+indexArgumento2] ;
	}
	indexArgumento++;
	argv2[indexArgumento2] = '\0';
	return 1;
}

/**
* Ejecuta el Pipeline
* @param argv1 Argumentos del comando 1.
* @param argv2 Argumentos del comando 2.
* @param paths Paths donde puede buscar los archivos que ejecutan los comandos.
*/
void doPipeline ( char* argv1[] , char* argv2[] , char* paths[] ) {
	char executePath[400];
	int fd[2]; //pipe!
	pipe ( fd );
	if ( fork () == 0 ) { //codigo del hijo
		int err4=close ( fd[0] );
		if(err4<0){
			perror("Error en el cierre del archivo.");
			exit(1);
		}
		int err8=dup2 ( fd[1] , 1 ); // redireccion de la salida al pipe.
		if(err8<0){
			perror("Error en la duplicacion del fichero.");
			exit(1);
		}
		int err5=close ( fd[1] );
		if(err5<0){
			perror("Error en el cierre del archivo.");
			exit(1);
		}
		buscarArchivo ( argv1[0] , paths , executePath );
		execv ( executePath , argv1 );
		perror ( executePath );
		exit ( 1 );
	} 
	else {
		int err6= close ( fd[1] ); //codigo del padre
		if(err6<0){
			perror("Error en el cierre del archivo.");
			exit(1);
		}
		int err9= dup2 ( fd[0] , 0 );
		if(err9<0){
			perror("Error en la duplicacion del fichero.");
			exit(1);
		}
		int err7= close ( fd[0] );
		if(err7<0){
			perror("Error en el cierre del archivo.");
			exit(1);
		}

		buscarArchivo ( argv2[0] , paths, executePath );
		execv ( executePath , argv2 );
		perror ( executePath );
		exit ( 1 );
	}
}


/**
* Remplaza el /home/username por el caracter ~.
* @param directorioDeTrabajo Arreglo que contiene el directorio actual de trabajo.
* @return Devuelve el directorio a imprimir por consola.
*/
char *acondicionarHome ( char directorioDeTrabajo[] ) {
	char *dir = directorioDeTrabajo;
	char *pointer;
	int barra = ( int ) '/';
	pointer = strstr ( dir , "/home/" ); //Detecta presencia de /home/ en el directorio.
	if ( pointer != NULL ) {
		char *pointer2;
		pointer2 = strchr ( dir , barra );
		pointer2 = strchr ( pointer2+1 , barra );
		if ( pointer2 != NULL ) {
			pointer2 = strchr ( pointer2+1 , barra ); //Detecta la cantidad de barras "/".
			char *enie = "~";
			if ( pointer2 != NULL ) {
				
				strcpy(dir,enie);
				strcat(dir,pointer2);
				return dir;
			}
			else {
				
				strcpy(dir,enie);
				strcat(dir,"$");
				return dir;
			}

		}
		else {
			return dir;
		}
	}
	else {
		return dir;
	}
}

extern int errno ;

int main () {
	int pid, flagWaitPID, bgProcess;
	int argC;
	char* argV[20];
	char* argv1[20];       //Para el caso que se haga un pipeline.
	char* argv2[20];
	char executePath[256]; //Path del proceso que se va a ejecutar.
	char comando [256];    //Comando que ingresa el usuario.
	char actualdir[100];
	//Almaceno nombre de usuario logeado y nombre del equipo.
	char hostname [100];
	int err0=gethostname ( hostname ,100);
	if(err0<0){
		perror("Error en obtencion de hostname.");
		exit(1);
	}

	//Obtengo todos los paths que están en la variable de entorno PATH
	char *paths[20];
	getPaths ( paths );
	int volatile contadorHijos = 0; //Para saber el numero de hijos.

	while (!feof(stdin)) {
		strcpy ( comando , "\n" ); //Vacío el comando para que no se ejecute si se presiona Ctrl+D.
		flagWaitPID = 0; //Limpio las banderas
		getcwd ( actualdir , 100 );        //Directorio de trabajo almacenado en actualdir.
		if(actualdir==NULL){
			perror("Error en obtener directorio actual.");
			exit(1);
		}
		if(getlogin()==NULL){
			perror("Error en obtencion del login.");
			exit(1);
		}
		LOG_RED ( getlogin() , hostname ); //Imprime el uid/gid
		strcat ( actualdir , " $" );
		char *dir = acondicionarHome ( actualdir );
		LOG_BLUE ( dir );
		fgets ( comando , 256 , stdin );

		//Si comando tiene solo un salto de línea, no hago nada.
		if( !strcmp ( comando , "\n" ) ) {
			continue;
		}
		//Esto es lo que se ejecuta si se ingresa algun comando
		else {
			argC = parse_Command ( argV , comando );
		//Comandos internos del bash. 
			if ( !strcmp ( comando , "exit" ) )
				return 0;
			if( !strcmp (argV[0] , "cd" ) || !strcmp (argV[0] , "~" )) {
				contadorHijos=0;
				if ( argC==1 ) {
					if(getenv ( "HOME" )==NULL){
						printf("%s\n","Error en obtener directorio HOME." );
						exit(1);
					}
					cdBuiltin ( getenv ( "HOME" ) );
				}
				if(argC>=2){ // Comando interno cd con directorios que presentan espacios en blanco (va a intentar unir todos los argumentos y abrir el directorio).
					char carpeta[100] = "";
					for ( int i = 1 ; i < argC ; ++i ) {
						strcat ( carpeta , argV[i] );
						if ( argC != i+1 ) {
						strcat ( carpeta , " " );
						}
					}
					cdBuiltin ( carpeta );
					continue;
				}
				continue;
			}

			if ( strcmp ( argV[0] , ".." ) == 0 ) {  //Ingreso de .. (Basado en Shell de Linux).
				contadorHijos=0;
				printf ( "%s\n" , "..: No se encontro la orden" );
				continue;
			}



			if (strcmp ( argV[0] , "&" ) == 0 ) {  //Ingreso de .. (Basado en Shell de Linux).
				contadorHijos=0;
				printf ( "%s\n" , "bash : error sintactico cerca del elemento inesperado \"&\"");
				continue;
			}

			int flagRedirect = 0;
			int doPipe = 0;
			char fileName[50];
			

			//Verifico si el proceso se tiene que ejecutar en Background.
			bgProcess = background ( argV , argC );
			if ( bgProcess == 1 ) {
				argV[argC-1] = NULL;
				argC--;
			}
			if ( bgProcess == 2 ) { //no hay que cortar los argumentos ni reducir argc
				bgProcess=1;
			}
			doPipe = checkPipe ( argV , argv1 , argv2 );
			flagRedirect = checkRedirect ( argV , fileName );
			buscarArchivo ( argV[0] , paths , executePath );
			if ( executePath[0] == 'D' ){
				contadorHijos=0;
				printf("baash: %s : Is a directory.\n",argV[0]);
				continue;
			}
			if ( executePath[0] == 'X' ){
				printf("baash: %s : No se encontro el archivo.\n",argV[0] );
				contadorHijos=0;
			}
			else {
				int pipeExecuted = 0;
				contadorHijos++;
				pid = fork();
				if ( pid < 0 ) {
					perror ( "Error al hacer el fork");
					exit ( 1 );
				}
				else if ( pid == 0 ) {  //Proceso hijo.
					if ( flagRedirect == 2 ) {   //comprueba las banderas de redireccion de entrada/salida
						outPut ( fileName );
					}
					else if ( flagRedirect == 1 ) {           
						freopen(fileName,"r",stdin);

					}
					else if ( doPipe == 1 ) {
						doPipeline ( argv1 , argv2 , paths );
						pipeExecuted = 1;
					}
					if ( !pipeExecuted ) {
						execv ( executePath , argV );
						perror ( executePath );
						exit ( 1 ) ;
					}
				}
				//Proces padre.
				else { 
					flagWaitPID = -1;
					waitpid( -1 , NULL , WNOHANG );

				}
				if ( bgProcess ) {
					printf ( "[%d]   %d\n", contadorHijos , pid ); // para imprimir [Id] hijo
				}
				else {
					waitpid ( pid , &flagWaitPID , 0 ); // No espero!
					contadorHijos=0;
				}
			}
		}
	}
	printf("\n");
	return 0;
}
