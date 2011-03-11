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
	unsigned int portNo;
	unsigned int location;
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
	list<struct beaconList *> *myBeaconList;
};

extern struct myStartInfo *myInfo;

extern struct myStartInfo* parseINIfile(unsigned char *fileName);


