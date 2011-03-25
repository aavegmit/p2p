#include<stdio.h>
#include<stdlib.h>
#include "main.h"
#include "iniParser.h"


void* connectBeacon(void *args)
{		
	struct node n;
	memset(&n, 0, sizeof(n));
	n = *((struct node *)args);
	//x = (struct node *)args;
	//printf("size of List is: %s %d\n", n.hostname, n.portNo);
	//free(args);

	//n.portNo = (((struct node *)args).portNo) ;
	//strcpy(n.hostname, (const char *)(args.hostname) ;
	list<pthread_t > myChildThreadList;
	int resSock = 0;
	int res = 0;
	while(!shutDown){
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		if (nodeConnectionMap.find(n)!=nodeConnectionMap.end()){
			pthread_mutex_unlock(&nodeConnectionMapLock) ;
		}
		else {

			pthread_mutex_unlock(&nodeConnectionMapLock) ;

			printf("Connecting to %s:%d\n", n.hostname, n.portNo) ;
			//sleep(1);
			resSock = connectTo((unsigned char *)n.hostname, n.portNo) ;
			if (resSock == -1 ){
				// Connection could not be established
			}
			else{
				struct connectionNode cn ;
				int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
				if (mres != 0){
					perror("Mutex initialization failed");
				}
				int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
				if (cres != 0){
					perror("CV initialization failed") ;
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
					perror("Thread creation failed");
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
					perror("Thread creation failed");
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
						perror("Thread join failed");
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
		sleep(myInfo->retry) ;
		//printf("Hello I am here!!!\n");

	}
	//printf("I am gone!!!\n");
	myChildThreadList.clear();
	pthread_exit(0);
}
