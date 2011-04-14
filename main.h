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
#include <queue>

#define HEADER_SIZE 27
#define SHA_DIGEST_LENGTH 20

using namespace std ;

//keeps the join responses
extern set<struct joinResNode> joinResponse ;
//keeps the status messgae
extern set< set<struct node> > statusResponse ;
extern map<struct node, list<string> > statusResponseTypeFiles ;

struct node{
	unsigned short int portNo;
	unsigned char hostname[256];

	bool operator<(const struct node& node1) const{
		return node1.portNo < portNo ;
	}
	bool operator==(const struct node& node1) const{
		return ((node1.portNo == portNo) && !strcmp((char *)node1.hostname, (char *)hostname)) ;
	}
};

struct joinResNode{
	unsigned short int portNo;
	unsigned char hostname[256];
	uint32_t location ;

	bool operator<(const struct joinResNode& node1) const{
		return node1.location > location ;
	}
	bool operator==(const struct joinResNode& node1) const{
		return ((node1.portNo == portNo) && !strcmp((char *)node1.hostname, (char *)hostname) && (node1.location == location)) ;
	}
};

extern map<struct node, int> nodeConnectionMap ;				// To store all the neighboring nodes
extern pthread_mutex_t nodeConnectionMapLock ;

//message structure that goes into the messgae queues
struct Message{
	uint8_t type;
	unsigned char *buffer ;
	unsigned char *query ;
	uint8_t ttl ;
	uint32_t location ;
	int status ;								// 0 - originated from here
	unsigned char uoid[SHA_DIGEST_LENGTH] ;
	bool fromConnect ;							// 1 - The message was created by the node which 
										// initiated the connection
	int buffer_len ;
	uint8_t status_type ;	
	unsigned char query_type ;
	uint8_t errorCode;
} ;

//information for the neighbors stored in this structures
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
	bool joinFlag;
	struct node n;
};

struct metaData
{
	unsigned int fileNumber;
	unsigned char fileID[20];
	unsigned char fileName[256];
	unsigned long int fileSize;
	unsigned char sha1[20];
	unsigned char nonce[20];
	list<string > *keywords;
	//map<string, int> keywords;
	unsigned char bitVector[128];
};

extern unsigned char tempLogFile[512], tempInitFile[512];
extern bool shutDown ;
extern int accept_pid;
extern int nSocket_accept;
extern int keepAlive_pid;
extern int toBeClosed;
extern int joinTimeOutFlag;
extern int inJoinNetwork;
extern int statusTimerFlag ;
extern int checkTimerFlag ;
extern int searchTimerFlag ;
extern int node_pid;
extern int softRestartFlag ;
extern int globalFileNumber ;
extern int globalSearchCount ;
extern FILE *f_log;
extern map<int, struct connectionNode> connectionMap ;				// Stores all the info related to a connection
extern list<pthread_t > childThreadList ;
extern map<pthread_t , bool > myConnectThread ;
extern map<string, list<int> > bitVectorIndexMap;
extern map<string, list<int> > fileNameIndexMap;
extern map<string, list<int> > sha1IndexMap;
extern map<string, int> fileIDMap;
extern map<int, struct metaData> getFileIDMap;
extern list<int > cacheLRU;
//extern list<struct metaData> metadataList;
extern pthread_mutex_t connectionMapLock ;
extern pthread_mutex_t statusMsgLock ;
extern pthread_mutex_t searchMsgLock ;
extern pthread_mutex_t logEntryLock ;
extern pthread_cond_t statusMsgCV;
extern pthread_cond_t searchMsgCV;

//pakcet structre stored in the cache at the nodes
struct Packet{

	struct node receivedFrom ;
	int status;								//  0 - originally Sent from here, 
										//  1 - Forwarded from here, route back, else -1
	int sockfd ;
	int msgLifeTime;	
	int status_type ;
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
void notifyMessageSend(int resSock, uint8_t errorCode);
void pushMessageinQ(int, struct Message ) ;
void closeConnection(int) ;
void joinNetwork() ;
void getStatus() ;
void writeToStatusFile() ;
void getStatusTypeFiles() ;
void writeToStatusFile_TypeFiles() ;
unsigned char *createLogEntry(unsigned char mode, int origin, unsigned char header[HEADER_SIZE], unsigned char *buffer);
void writeLogEntry(unsigned char *logEntry);
void eraseValueInMap(int val);
extern unsigned char *GetUOID(char *, unsigned char *, long unsigned int) ;
void initiateCheck() ;
void init() ;
void cleanup() ;
void writeMetaData(struct metaData metadata);
void updateGlobalFileNumber();
void writeData(struct metaData);
void populateBitVectorIndexMap(unsigned char*, unsigned int);
void populateSha1IndexMap(unsigned char*, unsigned int);
void populateFileNameIndexMap(unsigned char*, unsigned int);
unsigned char* toHex(unsigned char *str, int len);
struct metaData populateMetaData(int fileNumber);
void initiateSearch(unsigned char, unsigned char *) ;
string MetaDataToStr(struct metaData) ;
struct metaData populateMetaDataFromString(unsigned char *input);
int searchResponseDisplay(list<struct metaData> metadataList, int count);
void writeLRUToFile();
void readLRUFromFile();
void updateLRU(int fileNumber);
list<int> getAllFiles() ;
