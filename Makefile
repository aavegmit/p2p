# Macros

CC = g++
CFLAGS = -g -Wall -D_REENTRANT
OBJ = main.o keyboard.o timer.o iniParser.o incoming_connections.o outgoing_connections.o
LIBS = -lcrypto -lpthread 
INC = 
#LIBS = -L/home.scf-22/csci551b/openssl/lib -lcrypto -lnsl -lsocket -lresolv
#INC = -I/home/scf-22/csci551b/openssl/include

# Explicit rule
all: sv_node

sv_node: $(OBJ)
	$(CC) $(CFLAGS) -o sv_node $(OBJ) $(INC) $(LIBS) 
	cp sv_node bnode1/
	cp sv_node bnode2/
	cp sv_node bnode3/

clean:
	rm -rf *.o sv_node 

main.o: main.cc
	$(CC) $(CFLAGS) -c main.cc $(INC)
keyboard.o: keyboard.cc
	$(CC) $(CFLAGS) -c keyboard.cc $(INC)
timer.o: timer.cc
	$(CC) $(CFLAGS) -c timer.cc $(INC)
iniParser.o: iniParser.cc
	$(CC) $(CFLAGS) -c iniParser.cc $(INC)
incoming_connections.o: incoming_connections.cc
	$(CC) $(CFLAGS) -c incoming_connections.cc $(INC)
outgoing_connections.o: outgoing_connections.cc
	$(CC) $(CFLAGS) -c outgoing_connections.cc $(INC)
