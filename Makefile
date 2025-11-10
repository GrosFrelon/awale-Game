.PHONY: all
all: server.app client.app

server.app: Serveur/awale.o Serveur/liste_chaine.o Serveur/server.o Serveur/socket.o
	gcc -o server.app Serveur/awale.o Serveur/liste_chaine.o Serveur/server.o Serveur/socket.o

client.app: Client/client.c Client/client.h Client/jeu.h Client/player.h
	gcc -o client.app Client/client.c

Serveur/awale.o: Serveur/awale.c Serveur/awale.h Serveur/constants.h
	cd Serveur/ && gcc -c awale.c

Serveur/liste_chaine.o: Serveur/liste_chaine.c Serveur/liste_chaine.h Serveur/client.h Serveur/game.h
	cd Serveur/ && gcc -c liste_chaine.c

Serveur/server.o: Serveur/server.c Serveur/server.h Serveur/liste_chaine.h Serveur/socket.h Serveur/client.h Serveur/game.h Serveur/types.h Serveur/awale.h Serveur/player.h
	cd Serveur/ && gcc -c server.c

Serveur/socket.o: Serveur/socket.c Serveur/socket.h Serveur/awale.h Serveur/client.h Serveur/types.h
	cd Serveur/ && gcc -c socket.c

.PHONY: clean
clean:
	@echo "Cleaning..."
	rm -f server.app client.app
	rm -f Serveur/*.o
	rm -f Client/*.o