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

using namespace std;

struct beaconList
{
	unsigned char hostName[256];
	unsigned short int portNo;
};

struct myStartInfo
{
	unsigned char hostName[256];
	unsigned short int portNo;
	uint32_t location;
	unsigned char homeDir[512];
	unsigned char logFileName[256];
	unsigned int autoShutDown;
	unsigned int autoShutDown_permanent;	
	uint8_t ttl;
	unsigned int msgLifeTime;
	unsigned int getMsgLifeTime;
	unsigned int initNeighbor;
	unsigned int joinTimeOut;
	unsigned int joinTimeOut_permanent;
	unsigned int keepAliveTimeOut;
	unsigned int minNeighbor;
	unsigned int noCheck;
	double cacheProb;
	double storeProb;
	double neighborStoreProb;
	unsigned int cacheSize;
	unsigned int retry;
	list<struct beaconList *> *myBeaconList;
	
	bool isBeacon;						// to check if the current node is beacon or not
	unsigned char node_id[265] ;					// To store the unique node ID of this node
	unsigned char node_instance_id[300];				// To store the node instance ID, node_id + time when node started
	uint8_t status_ttl ;
	unsigned char status_file[256] ;
	unsigned int statusResponseTimeout ;
	unsigned int checkResponseTimeout ;
};

extern struct myStartInfo *myInfo;

extern void parseINIfile(unsigned char *fileName);

extern void populatemyInfo();

void printmyInfo();

extern void setNodeInstanceId() ;

