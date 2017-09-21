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
#define LOG_RED(X) printf ( "%s %s %s" , Color_Red , X , Color_end )
#define LOG_GREEN(X) printf ( "%s %s %s" , Color_Green , X , Color_end )

char* concat(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
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
		paths[i] = concat( pathsaux[i] , "/" );
	}
	return pathcounter;
}



char * getRutaEjecucion ( char* paths[] , int cantidad , char* comando , char* clasificacion, char* actualdir ) {
	if ( strcmp(clasificacion,"comando") == 0 ) {
		for ( int i = 0 ; i < cantidad ; i++ ) {
			if ( access( concat ( paths[i] , comando ) , X_OK ) == 0 ) {
			return concat( paths[i] , comando );
			}
		}
	}
	else if ( strcmp(clasificacion,"absoluto") == 0 ){
		if ( access( comando , X_OK ) == 0 ) {
			return comando;
		}
	}
	else if ( strcmp(clasificacion,"relativo") == 0 ){
		if ( access( concat(concat(actualdir,"/"),comando) , X_OK ) == 0 ) {
			return concat(concat(actualdir,"/"),comando);
		}
	}
	else if ( strcmp(clasificacion,"relativo-r") == 0 ){
		if ( access( concat(concat(actualdir,"/"),comando) , X_OK ) == 0 ) {
			return concat(concat(actualdir,"/"),comando);
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
	int res;
	if ( strncmp ( argumento , "/" , 1 ) == 0 )
		res=chdir(argumento);
	else if ( strncmp( argumento , ".." , 2 ) == 0 )
		res=chdir(argumento);
	else{
		res=chdir(concat("./",argumento));
	}

	if( res != 0 ) {
		fprintf ( stderr , "ERRNO: %d\n" , errno );
    	perror ( "Error" );
	}
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
    while ( ! feof ( stdin ) ) {
    	getcwd(actualdir, 100);
    	LOG_RED(concat(concat(getlogin(),"@"),hostname)); //imprimo el uid/gid
    	LOG_BLUE(actualdir);
    	strcpy(buffer, "\n");
    	fgets(buffer,1024,stdin);
    	if (strcmp(buffer,"\n")==0) {
    		continue;
    	}
    	else{
    		argc=parseArguments(argv,buffer);
    		if (strcmp(argv[0],"cd") == 0 ){
    			cdBuiltin(argv[1]);
    			continue;
    		}
    		if (strcmp(argv[0],"exit") == 0 ){
    			return 0;
    		}
    		if (strcmp(argv[0],"..") == 0 ){
    			printf("%s\n", "..: No se encontro la orden");
    			continue;
    		}
    		clasificacion=getClasificacionComando( argv[0] );
    		ruta=getRutaEjecucion(paths,cantidadpaths,argv[0],clasificacion,actualdir);
    		if (strcmp(ruta,"[error]") != 0 ){
    			printf("%s\n", "me re forkee");
    			pid = fork();
    			if (pid == 0) {
    				ejecutarArchivo( ruta , argv );
    				exit(0);
    			}
    			else
    				wait(0);
    		}

    	}		
    }
	return 0;
}