#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <list>
#include <unistd.h>
#include <ctype.h>
#include <map>
#include <openssl/sha.h>

#define HEADER_SIZE 27

using namespace std ;

struct node{
	unsigned int portNo;
	char hostname[256];

	bool operator<(const struct node& node1) const{
		return node1.portNo < portNo ;
	}
};

extern map<struct node, int> nodeConnectionMap ;


struct Message{
	uint8_t type;
	unsigned long int location ;

} ;

struct connectionNode{
	list<struct Message> MessageQ ;
	pthread_mutex_t mesQLock ;
	pthread_cond_t mesQCv ;
	int shutDown  ;

};

extern bool shutDown ;
extern map<int, struct connectionNode> connectionMap ;

struct Packet{

	struct node receivedFrom ;
	int status;			//  0 - Sent, 1 - Forwarded, else -1
};

extern map<unsigned char *, struct Packet> MessageDB ;


// Thread function declarations
void *keyboard_thread(void *) ;
void *timer_thread(void *) ;
void *accept_connectionsT(void *);	// Waits for other nodes to connect
void *read_thread(void *) ;
void *write_thread(void *) ;


// Function declarations
int isBeaconNode(struct node n);
int connectTo(unsigned char *, unsigned int) ;
//void pushMessageinQ(int, uint8_t, unsigned long int) ;
void pushMessageinQ(int, struct Message ) ;
void closeConnection(int) ;
void joinNetwork() ;
extern unsigned char *GetUOID(char *, unsigned char *, int) ;


