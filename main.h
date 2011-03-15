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
#define SHA_DIGEST_LENGTH 20

using namespace std ;

struct node{
	unsigned int portNo;
	char hostname[256];

	bool operator<(const struct node& node1) const{
		return node1.portNo < portNo ;
	}
	bool operator==(const struct node& node1) const{
		return ((node1.portNo == portNo) && !strcmp(node1.hostname, hostname)) ;
	}
};

extern map<struct node, int> nodeConnectionMap ;				// To store all the neighboring nodes


struct Message{
	uint8_t type;
	unsigned char *buffer ;
	unsigned int ttl ;
	unsigned long int location ;
	int status ;								// 0 - originated from here
	unsigned char uoid[SHA_DIGEST_LENGTH] ;
	bool fromConnect ;							// 1 - The message was created by the node which 
										// initiated the connection
	int buffer_len ;

} ;

struct connectionNode{
	list<struct Message> MessageQ ;
	pthread_mutex_t mesQLock ;
	pthread_cond_t mesQCv ;
	int shutDown  ;
	unsigned int keepAliveTimer;
	unsigned int keepAliveTimeOut;	
};

extern bool shutDown ;
extern int accept_pid;
extern int joinTimeOutFlag;
extern map<int, struct connectionNode> connectionMap ;				// Stores all the info related to a connection

struct Packet{

	struct node receivedFrom ;
	int status;								//  0 - originally Sent from here, 
										//  1 - Forwarded from here, route back, else -1
	int sockfd ;
};

//extern map<unsigned char *, struct Packet> MessageDB ;			// Keeps a track of all the messages it sends/forwards
extern map<string, struct Packet> MessageDB ;					// Keeps a track of all the messages it sends/forwards


// Thread function declarations
void *keyboard_thread(void *) ;
void *timer_thread(void *) ;
void *accept_connectionsT(void *);						// Waits for other nodes to connect
void *read_thread(void *) ;
void *write_thread(void *) ;


// Function declarations
int isBeaconNode(struct node n);
int connectTo(unsigned char *, unsigned int) ;
//void pushMessageinQ(int, uint8_t, unsigned long int) ;
void pushMessageinQ(int, struct Message ) ;
void closeConnection(int) ;
void joinNetwork() ;
extern unsigned char *GetUOID(char *, unsigned char *, long unsigned int) ;


