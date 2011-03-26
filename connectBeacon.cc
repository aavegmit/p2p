#include<stdio.h>
#include<stdlib.h>
#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"


//Connect thread of every beacon node
//retries the connection, if connection broke, untill called for shutdown

void* connectBeacon(void *args)
{

pthread_t myID = pthread_self();
myConnectThread[myID] = 0;		
	struct node n;
	memset(&n, 0, sizeof(n));
	n = *((struct node *)args);
	
	//stroes the read & write child of the connect thread
	list<pthread_t > myChildThreadList;
	
	int resSock = 0;
	int res = 0;
	while(!shutDown){
	
	signal(SIGUSR2, my_handler);
	
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		if (nodeConnectionMap.find(n)!=nodeConnectionMap.end()){
			pthread_mutex_unlock(&nodeConnectionMapLock) ;
		}
		else {

			pthread_mutex_unlock(&nodeConnectionMapLock) ;

			//printf("Connecting to %s:%d\n", n.hostname, n.portNo) ;
			//sleep(1);
			resSock = connectTo((unsigned char *)n.hostname, n.portNo) ;
			if (resSock == -1 ){
				// Connection could not be established
			}
			else{
				struct connectionNode cn ;
				int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
				if (mres != 0){
					
					//perror("Mutex initialization failed");
					writeLogEntry((unsigned char *)"//Mutex lock initilization failed\n");
				}
				int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
				if (cres != 0){
					//perror("CV initialization failed") ;
					writeLogEntry((unsigned char *)"//CV initilization failed\n");
				}

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
				// Create a read thread for this connection
				pthread_t re_thread ;
				res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
				if (res != 0) {
					//perror("Thread creation failed");
					writeLogEntry((unsigned char *)"//Read Thread Creation failed!!!!\n");
					exit(EXIT_FAILURE);
				}
				myChildThreadList.push_front(re_thread);
				pthread_mutex_lock(&connectionMapLock) ;
				connectionMap[resSock].myReadId = re_thread;
				pthread_mutex_unlock(&connectionMapLock) ;
				// Create a write thread
				pthread_t wr_thread ;
				res = pthread_create(&wr_thread, NULL, write_thread , (void *)resSock);
				if (res != 0) {
					//perror("Thread creation failed");
					writeLogEntry((unsigned char *)"//Write Thread Creation failed!!!!\n");					
					exit(EXIT_FAILURE);
				}
				myChildThreadList.push_front(wr_thread);
				pthread_mutex_lock(&connectionMapLock) ;
				connectionMap[resSock].myReadId = wr_thread;
				pthread_mutex_unlock(&connectionMapLock) ;


				//pthread Join here
				void* thread_result;
				for (list<pthread_t >::iterator it = myChildThreadList.begin(); it != myChildThreadList.end(); ++it){
					//printf("Value is : %d and SIze: %d\n", (int)(*it), (int)childThreadList.size());
					res = pthread_join((*it), &thread_result);
					if (res != 0) {
						//writeLogEntry((char *)"//Thread Creation failed!!!!\n");
						//exit(EXIT_FAILURE);
					}
					it = myChildThreadList.erase(it);
					--it;
				}
				if(shutDown)
					break;
			}
		}//end of else

		if(shutDown)
			break;
			
		// Wait for 'retry' time before making the connections again
		myConnectThread[myID] = 1;
		sleep(myInfo->retry) ;
		myConnectThread[myID] = 0;
		//printf("Hello I am here!!!\n");

	}
	//printf("I am gone!!!\n");
	myChildThreadList.clear();
	pthread_exit(0);
}
