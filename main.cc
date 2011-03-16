#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <signal.h>
#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"
#include "keepAliveTimer.h"

int resetFlag = 0;
bool shutDown = 0 ;
int accept_pid;
int keepAlive_pid;
int toBeClosed;
int joinTimeOutFlag = 0;
int inJoinNetwork = 0;
int node_pid;
int nSocket_accept = 0;
unsigned char *fileName = NULL;
struct myStartInfo *myInfo ;
map<int, struct connectionNode> connectionMap;
map<struct node, int> nodeConnectionMap;
list<pthread_t > childThreadList ;
set<struct joinResNode> joinResponse ;

pthread_mutex_t connectionMapLock ;
pthread_mutex_t nodeConnectionMapLock ;
pthread_mutex_t MessageDBLock ;

void my_handler(int nSig);

int usage()
{
	printf("Usage is: \"sv_node [-reset] <.ini file>\"\n");
	return false;
}

int processCommandLine(int argc, char *argv[])
{
	int iniFlag=0;

	if(argc<2 || argc > 3)
		return usage();

	for(int i=1;i<argc;i++)
	{
		argv++;
		if(*argv[0]=='-')
		{
			if(strcmp(*argv,"-reset")!=0)
				return usage();
			else
				resetFlag = 1;
		}
		else
		{
			if(strstr(*argv, ".ini")==NULL)
				return usage();
			else
			{
				int len = strlen(*argv);
				fileName = (unsigned char *)malloc(sizeof(unsigned char)*(len+1));
				strncpy((char *)fileName, (char *)*argv, len);
				fileName[len+1]='\0';
				iniFlag = 1;
			}
		}
	}
	if(iniFlag!=1)
		return usage();
	else
		return true;
}


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
//	pthread_cancel(connectionMap[sockfd].myReadId);
//	pthread_cancel(connectionMap[sockfd].myWriteId);
	printf("This socket has been closed: %d\n", sockfd);
}




int main(int argc, char *argv[])
{
	if(!processCommandLine(argc, argv))
		exit(0);

	populatemyInfo();
	parseINIfile(fileName);
	free(fileName) ;
	memset(&accept_pid, 0, sizeof(accept_pid));

	node_pid = getpid();
	signal(SIGTERM, my_handler);
	//Checks if the node is beacon or not
	struct node n;
	strcpy(n.hostname, (char *)myInfo->hostName);
	n.portNo=myInfo->portNo;
	myInfo->isBeacon = isBeaconNode(n);
	
	//printmyInfo();
	//exit(0);
	
	// Assign a node ID and node instance id to this node
	{
		char host1[256] ;
		memset(host1, '\0', 256) ;
		gethostname(host1, 256) ;
		host1[255] = '\0' ;
		sprintf((char *)myInfo->node_id, "%s_%d", host1, myInfo->portNo) ;
		printf("My node ID: %s\n", myInfo->node_id) ;
		setNodeInstanceId() ;
	}

	// Initialize Locks
	int lres = pthread_mutex_init(&connectionMapLock, NULL) ;
	if (lres != 0){
		perror("Mutex initialization failed") ;
	}
	
	lres = pthread_mutex_init(&MessageDBLock, NULL) ;
	if (lres != 0){
		perror("Mutex initialization failed") ;
	}
	
	lres = pthread_mutex_init(&nodeConnectionMapLock, NULL) ;
	if (lres != 0){
		perror("Mutex initialization failed") ;
	}


	void *thread_result ;


	// Populate the structure manually for testing
	//	myInfo = (struct myStartInfo *)malloc(sizeof(struct myStartInfo)) ;
	//	myInfo->myBeaconList = new list<struct beaconList *> ;

	//myInfo->isBeacon = true ;

	//Added by Aaveg, Commented by Manu
	/*
	if (myInfo->portNo == 12318)
		myInfo->isBeacon = false;
	else
		myInfo->isBeacon = true ;
	*/
	//myInfo->isBeacon = false ;
	//	myInfo->portNo = 12347 ;
	//	myInfo->keepAliveTimeOut = 10 ;
	//
	//	struct beaconList *b1;
	//	b1 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
	//	strcpy((char *)b1->hostName, "localhost") ;
	//	b1->portNo = 12345 ;
	//	myInfo->myBeaconList->push_front(b1) ;
	//
	//	b1 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
	//	strcpy((char *)b1->hostName, "localhost") ;
	//	b1->portNo = 12346 ;
	//	myInfo->myBeaconList->push_front(b1) ;
	//
	//	b1 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
	//	strcpy((char *)b1->hostName, "localhost") ;
	//	b1->portNo = 12347 ;
	//	myInfo->myBeaconList->push_front(b1) ;

	// -------------------------------------------



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

		int resSock;
		while(tempBeaconList->size() > 0){
			for(list<struct beaconList *>::iterator it = tempBeaconList->begin(); it != tempBeaconList->end(); it++){
				struct node n;
				n.portNo = (*it)->portNo ;
				strcpy(n.hostname, (const char *)(*it)->hostName) ;
				pthread_mutex_lock(&nodeConnectionMapLock) ;
				if (nodeConnectionMap[n]){
					it = tempBeaconList->erase(it) ;
					--it ;
					pthread_mutex_unlock(&nodeConnectionMapLock) ;
					continue ;
				}
				pthread_mutex_unlock(&nodeConnectionMapLock) ;
				printf("Connecting to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
				if(shutDown)
				{
					shutdown(resSock, SHUT_RDWR);
					close(resSock);
					break;
				}
				resSock = connectTo((*it)->hostName, (*it)->portNo) ; 
				if(shutDown)
				{
					shutdown(resSock, SHUT_RDWR);
					close(resSock);
					break;
				}

				if (resSock == -1 ){
					// Connection could not be established
				}
				else{
					struct connectionNode cn ;
					struct node n;
					n.portNo = (*it)->portNo ;
					strcpy(n.hostname, (const char *)(*it)->hostName) ;
					it = tempBeaconList->erase(it) ;
					--it ;
//					nodeConnectionMap[n] = resSock ;

					int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
					if (mres != 0){
						perror("Mutex initialization failed");

					}
					int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
					if (cres != 0){
						perror("CV initialization failed") ;
					}

					cn.shutDown = 0 ;
					cn.keepAliveTimer = myInfo->keepAliveTimeOut/2;
					cn.keepAliveTimeOut = myInfo->keepAliveTimeOut;
					cn.isReady = 0;
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

					// Create a read thread for this connection
					pthread_t re_thread ;
					res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
					if (res != 0) {
						perror("Thread creation failed");
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
						perror("Thread creation failed");
						exit(EXIT_FAILURE);
					}
					childThreadList.push_front(wr_thread);
					pthread_mutex_lock(&connectionMapLock) ;
					connectionMap[resSock].myReadId = wr_thread;
					pthread_mutex_unlock(&connectionMapLock) ;
				}
			}
			// Wait for 'retry' time before making the connections again
			sleep(myInfo->retry) ;
		}


		// Join the accept connections thread

	}
	else{
		FILE *f;
		while(1)
		{
		
		char nodeName[256];
		printf("A regular node coming up...\n") ;

		//checking if the init_neighbor_list exsits or not

		f=fopen("init_neighbor_list", "r");
		sigset_t new_t;
		if(f==NULL)
		{
			printf("Neighbor List does not exist...Joining the network\n");
//			exit(EXIT_FAILURE);
	
			//Adding Signal Handler for USR1 signal
			accept_pid=getpid();
			signal(SIGUSR1, my_handler);
			
			inJoinNetwork = 1;	
			joinNetwork() ;	
			inJoinNetwork = 0;
			
			printf("Joining success\n");
			continue ;
		}

		myInfo->joinTimeOut = -1;
		sigemptyset(&new_t);
		sigaddset(&new_t, SIGUSR1);
		pthread_sigmask(SIG_BLOCK, &new_t, NULL);
		

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

		// Connect to neighbors in the list
		// File exist, now say "Hello"
		list<struct beaconList *> *tempNeighborsList;
		tempNeighborsList = new list<struct beaconList *>;
		struct beaconList *b2;
		//for(unsigned int i=0;i < myInfo->minNeighbor; i++)
		while(fgets(nodeName, 255, f)!=NULL)
		{
			//fgets(nodeName, 255, f);
			char *hostName = strtok(nodeName, ":");
			char *portNo = strtok(NULL, ":");

			b2 = (struct beaconList *)malloc(sizeof(struct beaconList)) ;
			strncpy((char *)b2->hostName, const_cast<char *>(hostName), strlen(hostName)) ;
			b2->hostName[strlen(hostName)]='\0';
			b2->portNo = atoi(portNo);
			tempNeighborsList->push_back(b2) ;

		}

		/*if(tempNeighborsList->size() != myInfo->minNeighbor)
		{
			printf("Not enough neighbors alive\n");
			// need to exit thread and do soft restart
			exit(EXIT_FAILURE);
		}*/

			if(tempNeighborsList->size() < myInfo->minNeighbor)
			{
				//need to delete file init_neighbor_list
				fclose(f);
				remove("init_neighbor_list");
				continue;
			}
			
			int nodeConnected = 0;
			int resSock;
			for(list<struct beaconList *>::iterator it = tempNeighborsList->begin(); it != tempNeighborsList->end(); it++){
				struct node n;
				n.portNo = (*it)->portNo ;
				strcpy(n.hostname, (const char *)(*it)->hostName) ;
				pthread_mutex_lock(&nodeConnectionMapLock) ;
				if (nodeConnectionMap[n]){
					it = tempNeighborsList->erase(it) ;
					--it ;
					pthread_mutex_unlock(&nodeConnectionMapLock) ;
					continue ;
				}
				pthread_mutex_unlock(&nodeConnectionMapLock) ;

				printf("Connecting to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
				if(shutDown)
				{
					shutdown(resSock, SHUT_RDWR);
					close(resSock);
					break;
				}
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
					strcpy(n.hostname, (const char *)(*it)->hostName) ;
					it = tempNeighborsList->erase(it) ;
					--it ;
//					nodeConnectionMap[n] = resSock ;

					int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
					if (mres != 0){
						perror("Mutex initialization failed");

					}
					int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
					if (cres != 0){
						perror("CV initialization failed") ;
					}
					//Shutdown initilazed to zero
					cn.shutDown = 0 ;
					cn.keepAliveTimer = myInfo->keepAliveTimeOut/2;
					cn.keepAliveTimeOut = myInfo->keepAliveTimeOut;
					cn.isReady = 0;
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
						perror("Thread creation failed");
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
						perror("Thread creation failed");
						exit(EXIT_FAILURE);
					}
					childThreadList.push_front(wr_thread);
					pthread_mutex_lock(&connectionMapLock) ;
					connectionMap[resSock].myReadId = wr_thread;
					pthread_mutex_unlock(&connectionMapLock) ;
				
					nodeConnected++;
				}
				if(nodeConnected == (int)myInfo->minNeighbor)
					break;
			}

		// Join the accept connections thread

		if(nodeConnected == (int)myInfo->minNeighbor || shutDown)
			break;
		else
		{
			//need to delete init_neighbor_node
			fclose(f);
			remove("init_neighbor_list");
			continue;
		}
			
		}
		fclose(f);
	}










	int res ;



	//KeepAlive Timer Thread, Sends KeepAlive Messages
	pthread_t keepAlive_thread ;
	res = pthread_create(&keepAlive_thread, NULL, keepAliveTimer_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(keepAlive_thread);
	// Call the Keyboard thread
	// Thread creation and join code taken from WROX Publications book
	
	keepAlive_pid = getpid();
	pthread_t k_thread ;
	res = pthread_create(&k_thread, NULL, keyboard_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(k_thread);
	// Call the timer thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t t_thread ;
	res = pthread_create(&t_thread, NULL, timer_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(t_thread);

	for (list<pthread_t >::iterator it = childThreadList.begin(); it != childThreadList.end(); ++it){
		//printf("Value is : %d and SIze: %d\n", (int)(*it), (int)childThreadList.size());
		res = pthread_join((*it), &thread_result);
		if (res != 0) {
			perror("Thread join failed");
			exit(EXIT_FAILURE);
			//continue;
		}
	}

	
	// Thread Join code taken from WROX Publications
	/*res = pthread_join(k_thread, &thread_result);
	if (res != 0) {
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}*/
	
printf("Complete Shutdown!!!!\n");
return 0;
}
