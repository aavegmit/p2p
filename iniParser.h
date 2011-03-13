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
	unsigned char hostName[256];
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
};

extern struct myStartInfo *myInfo;

extern void parseINIfile(unsigned char *fileName);

extern void populatemyInfo();

void printmyInfo();
