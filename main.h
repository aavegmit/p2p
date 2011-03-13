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

} ;

struct connectionNode{
	list<struct Message> MessageQ ;
	pthread_mutex_t mesQLock ;
	pthread_cond_t mesQCv ;

};

extern bool shutDown ;
extern map<int, struct connectionNode> connectionMap ;



// Function declarations
void *keyboard_thread(void *) ;
void *timer_thread(void *) ;
void *accept_connectionsT(void *);	// Waits for other nodes to connect
void *read_thread(void *) ;
void *write_thread(void *) ;


int isBeaconNode(struct node n);
int connectTo(unsigned char *, unsigned int) ;
void pushMessageinQ(int, uint8_t) ;
