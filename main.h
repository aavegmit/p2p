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
#include <stdlib.h>
#include <string.h>
#include <set>

#define HEADER_SIZE 27
#define SHA_DIGEST_LENGTH 20

using namespace std ;

extern set<struct joinResNode> joinResponse ;
extern set< set<struct node> > statusResponse ;


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

struct joinResNode{
	unsigned int portNo;
	char hostname[256];
	unsigned long int location ;

	bool operator<(const struct joinResNode& node1) const{
		return node1.location > location ;
	}
	bool operator==(const struct joinResNode& node1) const{
		return ((node1.portNo == portNo) && !strcmp(node1.hostname, hostname) && (node1.location == location)) ;
	}
};

extern map<struct node, int> nodeConnectionMap ;				// To store all the neighboring nodes
extern pthread_mutex_t nodeConnectionMapLock ;

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
	uint8_t status_type ;	
	int errorCode;
} ;

struct connectionNode{
	list<struct Message> MessageQ ;
	pthread_mutex_t mesQLock ;
	pthread_cond_t mesQCv ;
	int shutDown  ;
	unsigned int keepAliveTimer;
	int keepAliveTimeOut;	
	int myReadId;
	int myWriteId;
	int isReady;
};

extern bool shutDown ;
extern int accept_pid;
extern int nSocket_accept;
extern int keepAlive_pid;
extern int toBeClosed;
extern int joinTimeOutFlag;
extern int inJoinNetwork;
extern int statusTimerFlag ;
extern int node_pid;
extern FILE *f_log;
extern map<int, struct connectionNode> connectionMap ;				// Stores all the info related to a connection
extern list<pthread_t > childThreadList ;
extern pthread_mutex_t connectionMapLock ;
extern pthread_mutex_t statusMsgLock ;

struct Packet{

	struct node receivedFrom ;
	int status;								//  0 - originally Sent from here, 
										//  1 - Forwarded from here, route back, else -1
	int sockfd ;
	int msgLifeTime;	
};

extern map<string, struct Packet> MessageDB ;					// Keeps a track of all the messages it sends/forwards
extern pthread_mutex_t MessageDBLock ;

// Thread function declarations
void *keyboard_thread(void *) ;
void *timer_thread(void *) ;
void *accept_connectionsT(void *);						// Waits for other nodes to connect
void *read_thread(void *) ;
void *write_thread(void *) ;
void *connectBeacon(void *);

// Function declarations
int isBeaconNode(struct node n);
int connectTo(unsigned char *, unsigned int) ;
extern pthread_t k_thread;
//void pushMessageinQ(int, uint8_t, unsigned long int) ;
void notifyMessageSend(int resSock, int errorCode);
void pushMessageinQ(int, struct Message ) ;
void closeConnection(int) ;
void joinNetwork() ;
void getStatus() ;
void writeToStatusFile() ;
unsigned char *createLogEntry(unsigned char mode, int origin, unsigned char header[HEADER_SIZE], unsigned char *buffer);
void writeLogEntry(unsigned char *logEntry);
void eraseValueInMap(int val);
extern unsigned char *GetUOID(char *, unsigned char *, long unsigned int) ;


