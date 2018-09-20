CC=gcc
OBJ1 = serveur_3.o
SRC1 = serveur_3.c
DEPS1=serveur_UDP_3.h


all: exec_serv

exec_serv : $(OBJ1)
	$(CC) -o exec_serv $(OBJ1)


$(OBJ1) : $(SRC1) $(DEPS1)
	$(CC) -c $(SRC1) $(DEPS1)


clean :
	rm $(OBJ1) *.gch
	rm exec_serv
