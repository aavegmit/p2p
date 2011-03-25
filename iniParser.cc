#include<stdio.h>
#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<list>
#include <time.h>

#include "iniParser.h"
#include "main.h"

using namespace std;


void setNodeInstanceId(){
	// Should be called once node_id of the node is set
	time_t sec;
	sec = time (NULL) ;
	sprintf((char *)myInfo->node_instance_id, "%s_%ld", myInfo->node_id, (long)sec) ;
	printf("%s\n", myInfo->node_instance_id) ;
}



//Initilaizing default values for myInfo
void populatemyInfo()
{
	myInfo = (struct myStartInfo *)malloc(sizeof(myStartInfo));
	myInfo->myBeaconList = new list<struct beaconList *> ;
	myInfo->portNo = 0;
	gethostname((char *)myInfo->hostName, 256);
	myInfo->location = 0;
	memset(myInfo->homeDir, '\0', 512);
	strncpy((char *)myInfo->logFileName, "servant.log", strlen("servant.log"));
	myInfo->logFileName[strlen("servant.log")]='\0';
	myInfo->autoShutDown = 900;
	myInfo->autoShutDown_permanent = 900;
	myInfo->ttl = 30;
	myInfo->msgLifeTime = 30;
	myInfo->getMsgLifeTime = 300;
	myInfo->initNeighbor = 3;
	myInfo->joinTimeOut = 15;
	myInfo->joinTimeOut_permanent = myInfo->joinTimeOut;
	myInfo->checkResponseTimeout = 15;
	myInfo->keepAliveTimeOut = 60;
	myInfo->minNeighbor = 2;
	myInfo->noCheck = 0;
	myInfo->cacheProb = 0.1;
	myInfo->storeProb = 0.1;
	myInfo->neighborStoreProb = 0.2;
	myInfo->cacheSize = 500;
	myInfo->isBeacon = false;
	myInfo->retry = 30;
	memset(myInfo->node_id, '\0', 265) ;
	memset(myInfo->node_instance_id, '\0', 300) ;

}

//printing the values of myInfo
void printmyInfo()
{
	printf("Value for myInfo is:--\n\n");

	printf("Port No: %d\n",myInfo->portNo);
	printf("hostName: %s\n",myInfo->hostName);
	printf("Location: %d\n",myInfo->location);
	printf("HomeDir: %s\n",myInfo->homeDir);
	printf("logFileName: %s\n",myInfo->logFileName);
	printf("autoShutDown: %d\n",myInfo->autoShutDown);
	printf("TTL: %d\n",myInfo->ttl);
	printf("msgLifeTime: %d\n",myInfo->msgLifeTime);
	printf("getMsgLifeTime: %d\n",myInfo->getMsgLifeTime);
	printf("initNeighbor: %d\n",myInfo->initNeighbor);
	printf("joinTimeOut: %d\n",myInfo->joinTimeOut);
	printf("keepAliveTimeOut: %d\n",myInfo->keepAliveTimeOut);
	printf("minNeighbor: %d\n",myInfo->minNeighbor);
	printf("noCheck: %d\n",myInfo->noCheck);
	printf("cacheProb: %lf\n",myInfo->cacheProb);
	printf("storeProb: %lf\n",myInfo->storeProb);
	printf("neighborStoreProb: %lf\n",myInfo->neighborStoreProb);
	printf("cacheSize: %d\n",myInfo->cacheSize);
	printf("retry: %d\n",myInfo->retry);
	printf("isBeacon: %d\n",myInfo->isBeacon);

}

// removes white spaces from the line read from file
void removeSpaces(unsigned char *readLine)
{
	unsigned char tempreadLine[512];
	int j=0;

	for(int i=0;readLine[i]!='\0';i++)
	{
		if(readLine[i]=='\"')
		{
			i++;
			while(readLine[i]!='\"')
			{ 
				if(readLine[i]=='\0')
				{
					i--;
					break;
				}
				tempreadLine[j++]=readLine[i];
				i++;
			}
		}
		else
		{
			if(readLine[i]==' ' || readLine[i]=='\n' || readLine[i]=='\t' || readLine[i]=='\r')
				continue;
			else
			{
				tempreadLine[j++]=readLine[i];
			}
		}

	}
	tempreadLine[j]='\0';
	strncpy((char *)readLine, (char *)tempreadLine, strlen((char *)tempreadLine));
	readLine[strlen((char *)tempreadLine)]='\0';
}


// Parses the ini file passed as command line argument
void parseINIfile(unsigned char *fileName)
{
	FILE *f=fopen((char *)fileName,"r");
	int initFlag=0, beaconFlag=0;
	int visitedFlag[20];
	memset(&visitedFlag, 0, sizeof(visitedFlag));

	if(f==NULL)
	{
		printf("Error in file opening\n");
		exit(1);
	}


	//file passed, opened successfully
	unsigned char readLine[512];
	unsigned char *key, *value;

	while(fgets((char *)readLine, 511, f)!=NULL)
	{
		//removes trailing space, tabs, new line, CR
		removeSpaces(readLine);
		if(!(*readLine))
			continue;
		//printf("2. readLine is: %s\n", readLine);

		if(strcasecmp((char *)readLine, "[init]")==0)
		{
			if(visitedFlag[0]==0)
				visitedFlag[0]=1;
			else
			{
				visitedFlag[0]=2;
				continue;
			}

			initFlag = 1;
			beaconFlag=0;
			continue;
		}
		else
		{
			if(strcasecmp((char *)readLine, "[beacons]")==0)
			{
				if(visitedFlag[1]==0)
					visitedFlag[1]=1;
				else
				{
					visitedFlag[1]=2;
					continue;
				}

				beaconFlag=1;
				initFlag=0;
				continue;
			}
			else
			{
				key=(unsigned char *)strtok((char *)readLine,"=");

				if(initFlag)
				{
					if(!strcasecmp((char *)key, "port"))
					{
						if(visitedFlag[2]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[2]=1;
						}
						else
							continue;

						value=(unsigned char *)strtok(NULL,"=");
						myInfo->portNo=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "location"))
					{
						if(visitedFlag[3]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[3]=1;
						}
						else
							continue;

						value=(unsigned char *)strtok(NULL,"=");
						myInfo->location=atol((char *)value);
					}
					else if(!strcasecmp((char *)key, "homedir"))
					{
						if(visitedFlag[4]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[4]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						strncpy((char *)myInfo->homeDir,(char *)value, strlen((char *)value));
						myInfo->homeDir[strlen((char *)value)]='\0';
					}
					else if(!strcasecmp((char *)key, "logfilename"))
					{
						if(visitedFlag[5]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[5]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						strncpy((char *)myInfo->logFileName,(char *)value, strlen((char *)value));
						myInfo->logFileName[strlen((char *)value)]='\0';
					}
					else if(!strcasecmp((char *)key, "autoshutdown"))
					{
						if(visitedFlag[6]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[6]=1;
						}
						else
							continue;

						value=(unsigned char *)strtok(NULL,"=");
						myInfo->autoShutDown=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "ttl"))
					{
						if(visitedFlag[7]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[7]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->ttl=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "msglifetime"))
					{
						if(visitedFlag[8]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[8]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->msgLifeTime=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "getmsglifetime"))
					{
						if(visitedFlag[9]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[9]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->getMsgLifeTime=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "initneighbors"))
					{
						if(visitedFlag[10]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[10]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->initNeighbor=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "jointimeout"))
					{
						if(visitedFlag[11]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[11]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->joinTimeOut=atoi((char *)value);
						myInfo->checkResponseTimeout=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "keepalivetimeout"))
					{
						if(visitedFlag[12]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[12]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->keepAliveTimeOut=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "minneighbors"))
					{
						if(visitedFlag[13]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[13]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->minNeighbor=atoi((char *)value);
					}
					else if(!strcasecmp((char *)key, "nocheck"))
					{
						if(visitedFlag[14]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[14]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->noCheck=atoi((char *)value);
					}				
					else if(!strcasecmp((char *)key, "cacheprob"))
					{
						if(visitedFlag[15]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[15]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->cacheProb=atof((char *)value);
					}
					else if(!strcasecmp((char *)key, "storeprob"))
					{
						if(visitedFlag[16]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[16]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->storeProb=atof((char *)value);
					}
					else if(!strcasecmp((char *)key, "neighborstoreprob"))
					{
						if(visitedFlag[17]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[17]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->neighborStoreProb=atof((char *)value);
					}
					else if(!strcasecmp((char *)key, "cachesize"))
					{
						if(visitedFlag[18]==0)
						{
							if(visitedFlag[0]==2)
								continue;
							visitedFlag[18]=1;
						}
						else
							continue;
						value=(unsigned char *)strtok(NULL,"=");
						myInfo->cacheSize=atoi((char *)value);
					}
				}
				else
				{
					if(!strcasecmp((char *)key, "retry"))
					{
						if(visitedFlag[19]==0)
						{
							if(visitedFlag[1]==2)
								continue;
							visitedFlag[19]=1;
						}
						else
						{
							continue;
						}
						value=(unsigned char *)strtok(NULL, "=");
						myInfo->retry=atoi((char *)value);
					}
					else
					{
						value=(unsigned char *)strtok((char *)key, ":");
						struct beaconList *beaconNode = (struct beaconList *)malloc(sizeof(struct beaconList));
						//strncpy((char *)beaconNode->hostName, (char *)value, strlen((char *)value));
						for(unsigned int i=0;i<strlen((char *)value);i++)
							beaconNode->hostName[i] = value[i];
						beaconNode->hostName[strlen((char *)value)] = '\0';
						value=(unsigned char *)strtok(NULL,":");
						beaconNode->portNo = (unsigned int)atoi((char *)value);
						myInfo->myBeaconList->push_back(beaconNode);
						//printf("Host name is : %s\tHost Port no: %d\n", beaconNode->hostName, beaconNode->portNo);
					}

				}
			}
		}
	}
	fclose(f) ;
} 
