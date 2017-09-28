# makefile para el trabajo practico "Baash" de sistemas operativos, versión 1 
# usar tabulador (no espacios) en make

baash : baash.c
	gcc -Wall -Werror -pedantic -o baashEx baash.c
# -Wall nos mostrará todos los avisos que produzca el compilador, no solamente los errores. Los avisos nos indican dónde y/o porqué podría surgir algún error en nuestro programa.
# -pedantic nos aporta más información sobre los errores y los avisos mostrados por GCC.
# -Werror tratara a cualquier warning como si un error se tratase. 

clean:
	rm baash \
