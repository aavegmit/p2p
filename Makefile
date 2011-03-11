# Macros

CC = g++
CFLAGS = -g -Wall -D_REENTRANT
OBJ = main.o keyboard.o timer.o iniParser.o
LIBS = -lcrypto -lpthread
INC = 
#LIBS = -L/home.scf-22/csci551b/openssl/lib -lcrypto -lnsl -lsocket -lresolv
#INC = -I/home/scf-22/csci551b/openssl/include

# Explicit rule
all: sv_node

sv_node: $(OBJ)
	$(CC) $(CFLAGS) -o sv_node $(OBJ) $(INC) $(LIBS) 

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
