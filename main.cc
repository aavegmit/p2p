#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <signal.h>
#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"
#include "keepAliveTimer.h"
#include "indexSearch.h"

bool resetFlag = 0;
bool shutDown = 0 ;
int accept_pid;
int keepAlive_pid;
int toBeClosed;
int joinTimeOutFlag = 0;
int inJoinNetwork = 0;
int node_pid;
int nSocket_accept = 0;
int statusTimerFlag = 0 ;
int searchTimerFlag = 0 ;
int checkTimerFlag = 0 ;
//int globalFileNumber = 0;
unsigned char *fileName = NULL;
unsigned char tempLogFile[512], tempInitFile[512];
unsigned char extFile[256];
unsigned char filesDir[256];
int softRestartFlag = 0 ;
int globalSearchCount = 0 ;
int currentCacheSize = 0;

pthread_t k_thread;
FILE *f_log = NULL;
struct myStartInfo *myInfo ;
map<int, struct connectionNode> connectionMap;
map<struct node, int> nodeConnectionMap;
list<pthread_t > childThreadList ;
//list<pthread_t > myConnectThread ;
map<pthread_t, bool > myConnectThread;
set<struct joinResNode> joinResponse ;
set< set<struct node> > statusResponse ;
map< struct node, list<string> > statusResponseTypeFiles ;
map<string, list<int> > bitVectorIndexMap;
map<string, list<int> > fileNameIndexMap;
map<string, list<int> > sha1IndexMap;
map<string, int> fileIDMap;
list<int > cacheLRU;
map<int, struct metaData> getFileIDMap;
//list<struct metaData> metadataList;

pthread_mutex_t connectionMapLock ;
pthread_mutex_t nodeConnectionMapLock ;
pthread_mutex_t MessageDBLock ;
pthread_mutex_t statusMsgLock ;
pthread_mutex_t searchMsgLock ;
pthread_mutex_t logEntryLock ;
pthread_cond_t statusMsgCV;
pthread_cond_t searchMsgCV;

void my_handler(int nSig);
void resetValues();
unsigned char  *createBitVector(unsigned char *keyword);
list<int> keywordSearch(unsigned char *completeStr);
list<int> fileNameSearch(unsigned char *fileName);
list<int> sha1Search(unsigned char *sha1String);

int usage()
{
	printf("Incorrect command line argument--\n\nUsage is: \"sv_node [-reset] <.ini file>\"\n");
	return false;
}

// this function process the command line arguments, checks for reset options and colelcts the startup configuration file
int processCommandLine(int argc, unsigned char *argv[])
{
	int iniFlag=0;

	//not enough number of arguments
	if(argc<2 || argc > 3)
		return usage();

	for(int i=1;i<argc;i++)
	{
		argv++;
		if(*argv[0]=='-')
		{
			if(strcmp((char *)(*argv),"-reset")!=0)
				return usage();
			else
				resetFlag = 1;
		}
		else
		{
			if(strstr((char *)*argv, ".ini")==NULL)
				return usage();
			else
			{
				int len = strlen((char *)*argv);
				fileName = (unsigned char *)malloc(sizeof(unsigned char)*(len+1));
				strncpy((char *)fileName, (char *)*argv, len);
				fileName[len]='\0';
				iniFlag = 1;
			}
		}
	}
	if(iniFlag!=1)
		return usage();
	else
		return true;
}


//this functions closes the connection after shutting down the socket
void closeConnection(int sockfd){

	// Set the shurdown flag for this connection
	pthread_mutex_lock(&connectionMapLock) ;
	(connectionMap[sockfd]).shutDown = 1 ;
	pthread_mutex_unlock(&connectionMapLock) ;

	// Signal the write thread
	pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
	pthread_cond_signal(&connectionMap[sockfd].mesQCv) ;
	pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;

	// Finally, close the socket
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd) ;

	//printf("This socket has been closed: %d\n", sockfd);
	// Initiate a CHECK message
	if (!myInfo->isBeacon && !shutDown && !inJoinNetwork && !myInfo->noCheck){
		//printf("Inititating CHECK message\n") ;
				initiateCheck() ;
	}

}

// while closing a connection, this function is called to erase the value of neighbor from the list maintained
void eraseValueInMap(int val)
{
	for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
		if((*it).second == val)
		{
			nodeConnectionMap.erase((*it).first);
			break;
		}
	}
}


// Main body of the PEER TO PEER file sharing system
int main(int argc, char *argv[])
{

	map<string, list<int> > bitVectorIndexMap;
	map<string, list<int> > fileNameIndexMap;
	map<string, list<int> > sha1IndexMap;
	
	//Checks if the command arguments are correct or not
	if(!processCommandLine(argc, (unsigned char**)argv))
		exit(0);


	sigset_t new_t ;
	sigemptyset(&new_t);
	sigaddset(&new_t, SIGINT);
	pthread_sigmask(SIG_BLOCK, &new_t, NULL);
	
	//function populates the node elemenets with default values
	populatemyInfo();
	
	//parses the INI file, populates the 
	parseINIfile(fileName);
	free(fileName) ;
	
	myInfo->joinTimeOut_permanent = myInfo->joinTimeOut;
	myInfo->autoShutDown_permanent = myInfo->autoShutDown;
	
	time_t localtime=(time_t)0;
	
	time(&localtime);
	srand48((long)localtime);
	/******************************************************/
	/*struct metaData metadata = populateMetaData(33);
	metadataList.push_back(metadata);
	memset(&metadata, 0, sizeof(metadata));
	metadata = populateMetaData(34);
	metadataList.push_back(metadata);	
	searchResponseDisplay();

	unsigned char temp_str[20];
	for(map<int, string>::iterator it = getFileIDMap.begin(); it!=getFileIDMap.end();it++)
	{
		printf("%d ", (*it).first);
		printf("%s ", (((*it).second).c_str()));
		for(int i=0;i<20;i++)
		{
			temp_str[i] = ((*it).second).c_str()[i];
			printf("%02x", temp_str[i]);
		}
		printf("\n\n");
	}
		exit(0);*/
	/*populateBitVectorIndexMap(metadata.bitVector, 33);
	populateSha1IndexMap(metadata.sha1, 33);
	populateFileNameIndexMap(metadata.fileName, 33);*/

/*	writeIndexToFile();
*/
	//readIndexFromFile();
	//writeIndexToFile();
	/******************************************************/
	
	//creates the directory, homedirectory

//	memset(filesDir, 0, 256);
	sprintf((char *)filesDir, "%s/files", myInfo->homeDir);
	
	mkdir((char *)myInfo->homeDir, 0777);
	mkdir((char *)filesDir, 0777);
	unsigned char fileNumber_file[256];
	sprintf((char *)fileNumber_file, "%s/.fileNumber", myInfo->homeDir);
	FILE *f_fileNumber = fopen((char *)fileNumber_file, "r");
	if(f_fileNumber == NULL)
	{
		f_fileNumber = fopen((char *)fileNumber_file, "w");	
		fprintf(f_fileNumber, "%d", 0);
	}
	fclose(f_fileNumber);
	
	//Log file and init_neighbors_file
	memset(&tempLogFile, '\0', 512);
	memset(&tempInitFile, '\0', 512);
	strncpy((char *)tempLogFile, (char *)myInfo->homeDir, strlen((char*)myInfo->homeDir));
	strncpy((char *)tempInitFile, (char *)myInfo->homeDir, strlen((char*)myInfo->homeDir));	
	tempLogFile[strlen((char*)myInfo->homeDir)] = '\0';
	tempInitFile[strlen((char*)myInfo->homeDir)] = '\0';
	strcat((char *)tempLogFile, "/");
	strcat((char *)tempLogFile, (char *)myInfo->logFileName);
	strcat((char *)tempInitFile, "/init_neighbor_list");	

	readIndexFromFile();
	readLRUFromFile();

		//if reset option at the commnad prompt given, delets the log file, init neighbor file
	if(resetFlag)
	{
		//writeLogEntry((unsigned char *)"//Reset Option in command line is given, deleted Log file\n");
		remove((char *)tempLogFile);
		if(!myInfo->isBeacon)
			remove((char *)tempInitFile);
		deleteAllFiles();
	}
	//Opening the log file
	//f_log = fopen((char *)tempLogFile, "a");
	f_log = fopen((char *)myInfo->logFileName, "a");

	if(strcmp((char *)myInfo->homeDir, "\0")==0 || myInfo->portNo == 0 || myInfo->location == 0)
	{
		//Program should exit
		writeLogEntry((unsigned char *)"//Madatory elements of Nodes were not populated\n");
		exit(0);
	}
	
//	printf("Created a directory %s\n", (char *)myInfo->homeDir);
//		exit(0);
	//Checks if the node is beacon or not
	struct node n;
	//strcpy((char *)n.hostname, (char *)myInfo->hostName);
	for(int i=0;i<(int)strlen((char *)myInfo->hostName);i++)
		n.hostname[i] = myInfo->hostName[i];
	n.hostname[strlen((char *)myInfo->hostName)] = '\0';
	n.portNo=myInfo->portNo;
	myInfo->isBeacon = isBeaconNode(n);

	memset(&accept_pid, 0, sizeof(accept_pid));
	node_pid = getpid();
	signal(SIGTERM, my_handler);

	
	// Assign a node ID and node instance id to this node
	{
		unsigned char host1[256] ;
		memset(host1, '\0', 256) ;
		gethostname((char *)host1, 256) ;
		host1[255] = '\0' ;
		sprintf((char *)myInfo->node_id, "%s_%d", host1, myInfo->portNo) ;
	//	printf("My node ID: %s\n", myInfo->node_id) ;
		setNodeInstanceId() ;
	}

	// Initialize Locks and considiton vairables
	int lres = pthread_mutex_init(&logEntryLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");
	}
	lres = pthread_mutex_init(&connectionMapLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");		
	}

	lres = pthread_mutex_init(&MessageDBLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");		
	}

	lres = pthread_mutex_init(&nodeConnectionMapLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");
	}

	lres = pthread_mutex_init(&statusMsgLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");
	}

	int cres = pthread_cond_init(&statusMsgCV, NULL) ;
	if (cres != 0){
		//perror("CV initialization failed") ;
		writeLogEntry((unsigned char *)"//CV initialization failed\n");		
	}
	lres = pthread_mutex_init(&searchMsgLock, NULL) ;
	if (lres != 0){
		//perror("Mutex initialization failed") ;
		writeLogEntry((unsigned char *)"//Mutex initialization failed\n");
	}

	cres = pthread_cond_init(&searchMsgCV, NULL) ;
	if (cres != 0){
		//perror("CV initialization failed") ;
		writeLogEntry((unsigned char *)"//CV initialization failed\n");		
	}



	// Call the init function
	while(!shutDown || softRestartFlag){
	
		
		softRestartFlag = 0 ;
		signal(SIGTERM, my_handler);
		
		//Start of the logging 
		struct timeval tv;
		memset(&tv, 0, sizeof(tv));
		gettimeofday(&tv, NULL);
		unsigned char startLogging[256];
		memset(startLogging, '\0', 256);
		sprintf((char *)startLogging, "// %10ld.%03ld : Logging Started\n", tv.tv_sec, (tv.tv_usec/1000));
		writeLogEntry(startLogging);
	
		//Initialiing the threads, establishing connections
		init() ;
		
		//Cleaning up, shutting down
		cleanup() ;
		//printf("The SIze of master list is : %d\n", (int)childThreadList.size());
		//printf("going back again\n") ;
		
		//checks if the soft restart needs to be performed, and then reset the values
		if(softRestartFlag)
			resetValues();

		//Stop logging here		
		memset(&tv, 0, sizeof(tv));
		gettimeofday(&tv, NULL);
		unsigned char stopLogging[256];
		memset(stopLogging, '\0', 256);
		sprintf((char *)stopLogging, "// %10ld.%03ld : Logging Stopped\n", tv.tv_sec, (tv.tv_usec/1000));
		writeLogEntry(stopLogging);
		
		
	}


		writeIndexToFile();
		writeLRUToFile();

	//printf("Complete Shutdown!!!!\n");
	fclose(f_log);
	return 0;
}


void resetValues()
{
	shutDown = 0;
	myInfo->joinTimeOut = myInfo->joinTimeOut_permanent;
	myInfo->autoShutDown = myInfo->autoShutDown_permanent;
	myInfo->checkResponseTimeout = myInfo->joinTimeOut_permanent;
	checkTimerFlag = 0;
	nSocket_accept = 0;
	//remove((char *)tempInitFile);
}

void cleanup(){

	int res ;
	void *thread_result ;



	//KeepAlive Timer Thread, Sends KeepAlive Messages
	pthread_t keepAlive_thread ;
	res = pthread_create(&keepAlive_thread, NULL, keepAliveTimer_thread , (void *)NULL);
	if (res != 0) {
		//perror("Thread creation failed");
		writeLogEntry((unsigned char *)"//In Main: Thread creation failed\n");
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(keepAlive_thread);
	// Call the Keyboard thread
	// Thread creation and join code taken from WROX Publications book

	keepAlive_pid = getpid();
	//pthread_t k_thread ;
	res = pthread_create(&k_thread, NULL, keyboard_thread , (void *)NULL);
	if (res != 0) {
		//perror("Thread creation failed");
		writeLogEntry((unsigned char *)"//In Main: Thread creation failed\n");		
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(k_thread);
	// Call the timer thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t t_thread ;
	res = pthread_create(&t_thread, NULL, timer_thread , (void *)NULL);
	if (res != 0) {
		//perror("Thread creation failed");
		writeLogEntry((unsigned char *)"//In Main: Thread creation failed\n");		
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(t_thread);


	for (list<pthread_t >::iterator it = childThreadList.begin(); it != childThreadList.end(); ++it){
		//printf("Value is : %d and SIze: %d\n", (int)(*it), (int)childThreadList.size());
		res = pthread_join((*it), &thread_result);
		if (res != 0) {
			//perror("Thread join failed");
			writeLogEntry((unsigned char *)"//In Main: Thread joining failed\n");
			exit(EXIT_FAILURE);
			//continue;
		}
	}
	
	//clearing out the data structres used, if incase needed a restart
	nodeConnectionMap.clear();
	childThreadList.clear();
	connectionMap.clear();
	joinResponse.clear();
}


void init(){

	// If current node is beacon
	if (myInfo->isBeacon){
		// Call the Accept thread
		// Thread creation and join code taken from WROX Publications book
		pthread_t accept_thread ;
		int res ;
		res = pthread_create(&accept_thread, NULL, accept_connectionsT , (void *)NULL);
		if (res != 0) {
			perror("Thread creation failed");
			exit(EXIT_FAILURE);
		}
		childThreadList.push_front(accept_thread);

		// Connect to other beacon nodes
		list<struct beaconList *> *tempBeaconList ;
		tempBeaconList = new list<struct beaconList *> ;
		struct beaconList *b2;
		for(list<struct beaconList *>::iterator it = myInfo->myBeaconList->begin(); it != myInfo->myBeaconList->end(); it++){
			if ( (*it)->portNo != myInfo->portNo){
				b2 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
				strncpy((char *)b2->hostName, const_cast<char *>((char *)(*it)->hostName), 256) ;
				b2->portNo = (*it)->portNo ;
				tempBeaconList->push_front(b2) ;
			}
		}

		//creating the connect threads for the beaocn node, which will retry and retry, untill gets conected to form a complete mesh
		for(list<struct beaconList *>::iterator it = tempBeaconList->begin(); it != tempBeaconList->end() && shutDown!=1; ++it){
			//struct node *n = (struct node *)malloc(sizeof(n));
			struct node n;
			memset(&n, 0, sizeof(n));
			//n = NULL;
			n.portNo = (*it)->portNo ;
			//memcpy(&n.portNo, &(*it)->portNo, sizeof((*it)->portNo));
			//strncpy((char *)n->hostname, (const char *)(*it)->hostName, strlen((const char *)(*it)->hostName)) ;
			for (unsigned int i=0;i<strlen((const char *)(*it)->hostName);i++)
				n.hostname[i] = (*it)->hostName[i];
			n.hostname[strlen((const char *)(*it)->hostName)] = '\0';
			//memcpy(n.hostname, (*it)->hostName, strlen((char *)(*it)->hostName)) ;
			// Create a connect thread for this connection
			pthread_t connect_thread ;
			//printf("In Main: size of List is: %s %d\n", n->hostname, n->portNo);
			int res = pthread_create(&connect_thread, NULL, connectBeacon , (void *)&n);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}
			childThreadList.push_front(connect_thread);
			//myConnectThread[connect_thread] = 0;
			sleep(1);
		}	

	}
	else{
	
		//code for the non-beacon node
		FILE *f;
		while(!shutDown)
		{
	
			unsigned char nodeName[256];
			memset(&nodeName, '\0', 256);
			//printf("A regular node coming up...\n") ;
	
			//checking if the init_neighbor_list exsits or not
	
			f=fopen((char *)tempInitFile, "r");
			//f=fopen("init_neighbor_list", "r");
			sigset_t new_t;
			sigemptyset(&new_t);
			sigaddset(&new_t, SIGUSR1);
	
			if(f==NULL)
			{
				//printf("Neighbor List does not exist...Joining the network\n");
				//			exit(EXIT_FAILURE);
				writeLogEntry((unsigned char *)"//Initiating the Join process, since Init_Neighbor file not present\n");
	
				//Adding Signal Handler for USR1 signal
				accept_pid=getpid();
	
				pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);
	
				signal(SIGUSR1, my_handler);
	
				inJoinNetwork = 1;	
				joinNetwork() ;	
				inJoinNetwork = 0;
	
				//printf("Joining success\n");
				writeLogEntry((unsigned char *)"//Terminating the Join process, Init Neighbors identified\n");
				myInfo->joinTimeOut = myInfo->joinTimeOut_permanent;
				continue ;
			}
	
			myInfo->joinTimeOut = -1;
			pthread_sigmask(SIG_BLOCK, &new_t, NULL);


				// Call the Accept thread
				// Thread creation and join code taken from WROX Publications book
				pthread_t accept_thread ;
				int res ;
				res = pthread_create(&accept_thread, NULL, accept_connectionsT , (void *)NULL);
				if (res != 0) {
					//perror("Thread creation failed");
					writeLogEntry((unsigned char *)"//Accept Thread creation failed\n");
					exit(EXIT_FAILURE);
				}
				childThreadList.push_front(accept_thread);

				// Connect to neighbors in the list
				// File exist, now say "Hello"
				list<struct beaconList *> *tempNeighborsList;
				tempNeighborsList = new list<struct beaconList *>;
				struct beaconList *b2;
				//for(unsigned int i=0;i < myInfo->minNeighbor; i++)
				while(fgets((char *)nodeName, 255, f)!=NULL)
				{
					//fgets(nodeName, 255, f);
					unsigned char *hostName = (unsigned char *)strtok((char *)nodeName, ":");
					unsigned char *portNo = (unsigned char *)strtok(NULL, ":");

					b2 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
					strncpy((char *)b2->hostName, (char *)(hostName), strlen((char *)hostName)) ;
					b2->hostName[strlen((char *)hostName)]='\0';
					b2->portNo = atoi((char *)portNo);
					tempNeighborsList->push_back(b2) ;

				}

				int nodeConnected = 0;
				int resSock;
				for(list<struct beaconList *>::iterator it = tempNeighborsList->begin(); it != tempNeighborsList->end() && shutDown!=1; it++){
					struct node n;
					n.portNo = (*it)->portNo ;
					strncpy((char *)n.hostname, (const char *)(*it)->hostName, 256) ;
					pthread_mutex_lock(&nodeConnectionMapLock) ;
					//neighbor already exist in the neighbor list
					if (nodeConnectionMap.find(n)!=nodeConnectionMap.end()){
						nodeConnected++;
						it = tempNeighborsList->erase(it) ;
						--it ;
						pthread_mutex_unlock(&nodeConnectionMapLock) ;
						continue ;
					}
					pthread_mutex_unlock(&nodeConnectionMapLock) ;

					//printf("Connecting to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
					if(shutDown)
					{
						shutdown(resSock, SHUT_RDWR);
						close(resSock);
						break;
					}
					//trying to connect the host name and port no
					resSock = connectTo((*it)->hostName, (*it)->portNo) ; 
					if(shutDown)
					{
						shutdown(resSock, SHUT_RDWR);
						close(resSock);
						break;
					}				
					if (resSock == -1 ){
						// Connection could not be established
						// now we have to reset the network, call JOIN
						continue;
					}
					else{
						struct connectionNode cn ;
						struct node n;
						n.portNo = (*it)->portNo ;
						strncpy((char *)n.hostname, (const char *)(*it)->hostName, 256) ;
						it = tempNeighborsList->erase(it) ;
						--it ;
						//					nodeConnectionMap[n] = resSock ;

						int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
						if (mres != 0){
							//perror("Mutex initialization failed");
							writeLogEntry((unsigned char *)"//Mutex Initializtaion failed\n");

						}
						int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
						if (cres != 0){
							//perror("CV initialization failed") ;
							writeLogEntry((unsigned char *)"//CV Initializtaion failed\n");
						}
						//Shutdown initilazed to zero
						cn.shutDown = 0 ;
						cn.keepAliveTimer = myInfo->keepAliveTimeOut/3;
						cn.keepAliveTimeOut = myInfo->keepAliveTimeOut;
						cn.isReady = 0;
						cn.n = n;
						//signal(SIGUSR2, my_handler);

						pthread_mutex_lock(&connectionMapLock) ;
						connectionMap[resSock] = cn ;
						pthread_mutex_unlock(&connectionMapLock) ;
						// Push a Hello type message in the writing queue
						struct Message m ; 
						m.type = 0xfa ;
						m.status = 0 ;
						m.fromConnect = 1 ;
						pushMessageinQ(resSock, m) ;
						//					pushMessageinQ(resSock, 0xfa) ;

						// Create a read thread for this connection
						pthread_t re_thread ;
						res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
						if (res != 0) {
							//perror("Thread creation failed");
							writeLogEntry((unsigned char *)"//Read Thread creation failed\n");
							exit(EXIT_FAILURE);
						}
						pthread_mutex_lock(&connectionMapLock) ;
						connectionMap[resSock].myReadId = re_thread;
						childThreadList.push_front(re_thread);
						pthread_mutex_unlock(&connectionMapLock) ;

						// Create a write thread
						pthread_t wr_thread ;
						res = pthread_create(&wr_thread, NULL, write_thread , (void *)resSock);
						if (res != 0) {
							//perror("Thread creation failed");
							writeLogEntry((unsigned char *)"//Write Thread creation failed\n");
							exit(EXIT_FAILURE);
						}
						childThreadList.push_front(wr_thread);
						pthread_mutex_lock(&connectionMapLock) ;
						connectionMap[resSock].myWriteId = wr_thread;
						pthread_mutex_unlock(&connectionMapLock) ;

						nodeConnected++;
					}
					if(nodeConnected == (int)myInfo->minNeighbor)
						break;
				}

				//succesfully conected to minNeighbors
				if(nodeConnected == (int)myInfo->minNeighbor || shutDown)
					break;
				else
				{
					//coulndn't connect to the Min Neighbors, need to soft restart
					fclose(f);
					//remove((char *)tempInitFile);
					shutDown = 1;
					shutdown(nSocket_accept, SHUT_RDWR);
					close(nSocket_accept);
					
					//Send NOTIFY message beacouse of restart
					pthread_mutex_lock(&nodeConnectionMapLock);
					for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it)
						notifyMessageSend((*it).second, 3);
					pthread_mutex_unlock(&nodeConnectionMapLock);						
				
					sleep(1);
					
					pthread_mutex_lock(&nodeConnectionMapLock) ;
					for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it)
						closeConnection((*it).second);
					nodeConnectionMap.clear();
					pthread_mutex_unlock(&nodeConnectionMapLock) ;
					
					void *thread_result;
					for (list<pthread_t >::iterator it = childThreadList.begin(); it != childThreadList.end(); ++it){
						res = pthread_join((*it), &thread_result);
						if (res != 0) {
							perror("Thread join failed");
							exit(EXIT_FAILURE);
							//continue;
						}
					}
					childThreadList.clear();
					joinResponse.clear();
					//myInfo->joinTimeOut = myInfo->joinTimeOut_permanent;
					resetValues();
					continue;
				}

			}
			fclose(f);
		}


	}
