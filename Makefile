
PROGS = \
	client		\
	server

all : $(PROGS)

client : client.o 
	$(CC) -Wall -o $@ client.o -lm

server : server.o  
	$(CC) -Wall -o $@ server.o  -lm

server.o : server.c
	$(CC) -Wall -c server.c

client.o : client.c
	$(CC) -Wall -c client.c

clean :
	rm -f *.o $(PROGS)
