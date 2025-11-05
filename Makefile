server.app: Serveur/awale.o Serveur/liste_chaine.o Serveur/server.o Serveur/socket.o
	gcc -o server Serveur/awale.o Serveur/liste_chaine.o Serveur/server.o Serveur/socket.o

client.app: Client/client.c
	gcc -o client Client/client.c

Serveur/awale.o: Serveur/awale.c
	cd Serveur/ && gcc -c awale.c

Serveur/liste_chaine.o: Serveur/liste_chaine.c
	cd Serveur/ && gcc -c liste_chaine.c

Serveur/server.o: Serveur/server.c
	cd Serveur/ && gcc -c server.c

Serveur/socket.o: Serveur/socket.c
	cd Serveur/ && gcc -c socket.c
