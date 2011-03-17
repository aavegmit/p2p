#include<stdio.h>
#include<iostream>
#include<list>

using namespace std;

struct beaconList
{
	unsigned char hostName[256];
	unsigned int portNo;
};

struct myStartInfo
{
	unsigned char hostName[256];
	unsigned short int portNo;
	unsigned long int location;
	unsigned char homeDir[512];
	unsigned char logFileName[256];
	unsigned int autoShutDown;
	unsigned int ttl;
	unsigned int msgLifeTime;
	unsigned int getMsgLifeTime;
	unsigned int initNeighbor;
	unsigned int joinTimeOut;
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
	unsigned int status_ttl ;
	char status_file[256] ;
	unsigned int statusResponseTimeout ;
};

extern struct myStartInfo *myInfo;

extern void parseINIfile(unsigned char *fileName);

extern void populatemyInfo();

void printmyInfo();

extern void setNodeInstanceId() ;

