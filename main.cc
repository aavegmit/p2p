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

int resetFlag = 0;
bool shutDown = 0 ;
unsigned char *fileName = NULL;
struct myStartInfo *myInfo ;
map<int, struct connectionNode> connectionMap;
map<struct node, int> nodeConnectionMap;

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





int main(int argc, char *argv[])
{
	if(!processCommandLine(argc, argv))
		exit(0);

	populatemyInfo();
	parseINIfile(fileName);
	free(fileName) ;
	//	printmyInfo();

	void *thread_result ;

	// Call INI parser here - returns a structure

	// Populate the structure manually for testing
	//	myInfo = (struct myStartInfo *)malloc(sizeof(struct myStartInfo)) ;
	//	myInfo->myBeaconList = new list<struct beaconList *> ;

	myInfo->isBeacon = true ;
	//	myInfo->portNo = 12347 ;
	//	myInfo->retry = 4 ;
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

		while(tempBeaconList->size() > 0){
			for(list<struct beaconList *>::iterator it = tempBeaconList->begin(); it != tempBeaconList->end(); it++){
				printf("Connecting to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
				int resSock = connectTo((*it)->hostName, (*it)->portNo) ; 
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
					nodeConnectionMap[n] = resSock ;

					int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
					if (mres != 0){
						perror("Mutex initialization failed");
						
					}
					int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
					if (cres != 0){
						perror("CV initialization failed") ;
					}

					connectionMap[resSock] = cn ;
					// Push a Hello type message in the writing queue
					pushMessageinQ(resSock, 0xfa) ;

					// Create a read thread for this connection
					pthread_t re_thread ;
					res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
					if (res != 0) {
						perror("Thread creation failed");
						exit(EXIT_FAILURE);
					}

					// Create a write thread
					pthread_t wr_thread ;
					res = pthread_create(&wr_thread, NULL, write_thread , (void *)resSock);
					if (res != 0) {
						perror("Thread creation failed");
						exit(EXIT_FAILURE);
					}
				}
			}
			// Wait for 'retry' time before making the connections again
			sleep(myInfo->retry) ;
		}


		// Join the accept connections thread

	}
	else{
		printf("A regular node coming up...\n") ;
	}















	// Call the Keyboard thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t k_thread ;
	int res ;
	res = pthread_create(&k_thread, NULL, keyboard_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	// Call the timer thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t t_thread ;
	res = pthread_create(&t_thread, NULL, timer_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}


	// Thread Join code taken from WROX Publications
	res = pthread_join(k_thread, &thread_result);
	if (res != 0) {
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}



	return 0;
}
