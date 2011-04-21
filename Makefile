# Macros

CC = g++
CFLAGS = -g -Wall -D_REENTRANT
OBJ = main.o keyboard.o timer.o iniParser.o incoming_connections.o outgoing_connections.o signalHandler.o keepAliveTimer.o logEntry.o connectBeacon.o indexSearch.o metaData.o
LIBS = -lcrypto -lpthread
INC = 
#LIBS = -L/home.scf-22/csci551b/openssl/lib -lcrypto -lnsl -lsocket -lresolv
#INC = -I/home/scf-22/csci551b/openssl/include

# Explicit rule
all: sv_node

sv_node: $(OBJ)
	$(CC) $(CFLAGS) -o sv_node $(OBJ) $(INC) $(LIBS) 
#	cp sv_node bnode1/
#	cp sv_node bnode2/
#	cp sv_node bnode3/
#	cp sv_node nonbnode1/
#	cp sv_node nonbnode2/	

install:
#	cp sv_node bnode1/
#	cp sv_node bnode2/
#	cp sv_node bnode3/
#	cp sv_node nonbnode1/
#	cp sv_node nonbnode2/	

clean:
	rm -rf *.o sv_node 
#	rm bnode1/sv_node
#	rm bnode2/sv_node
#	rm bnode3/sv_node
#	rm nonbnode1/sv_node
#	rm nonbnode2/sv_node

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
signalHandler.o: signalHandler.cc
	$(CC) $(CFLAGS) -c signalHandler.cc $(INC)
keepAliveTimer.o: keepAliveTimer.cc
	$(CC) $(CFLAGS) -c keepAliveTimer.cc $(INC)
logEntry.o: logEntry.cc
	$(CC) $(CFLAGS) -c logEntry.cc $(INC)
connectBeacon.o: connectBeacon.cc
	$(CC) $(CFLAGS) -c connectBeacon.cc $(INC)
indexSearch.o: indexSearch.cc
	$(CC) $(CFLAGS) -c indexSearch.cc $(INC)
metaData.o: metaData.cc
	$(CC) $(CFLAGS) -c metaData.cc $(INC)
