#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "iniParser.h"
#include "main.h"
//#include <thread.h>


void *timer_thread(void *arg){

	while(1){

		sleep(1) ;

		if(shutDown)
		{
			printf("Timer thread halted\n");
			break;
		}
		//JoinTimeOut
		if(!myInfo->isBeacon && inJoinNetwork)
		{
			if(myInfo->joinTimeOut > 0)
				myInfo->joinTimeOut--;
			else
			{
				if(myInfo->joinTimeOut > 0)
					myInfo->joinTimeOut--;
				else
				{
					if(myInfo->joinTimeOut==0)
					{
						//printf("Timer out..: %d\n", (int)pthread_self()) ;
						kill(accept_pid, SIGUSR1);
						break;
						//myInfo->joinTimeOut--;
					}
				}
			}
		}
		//KeepAliveTimeOut
		pthread_mutex_lock(&connectionMapLock) ;
		if(!connectionMap.empty() && !inJoinNetwork)
		{
			for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it){
				if((*it).second.keepAliveTimeOut > 0)
					(*it).second.keepAliveTimeOut--;
				else
				{
					if((*it).second.keepAliveTimeOut == 0)
					{
						printf("Erasing entry from Map for : %d, %d\n", (*it).first, connectionMap[(*it).first].keepAliveTimeOut);
						(*it).second.keepAliveTimeOut=-1;
						pthread_mutex_unlock(&connectionMapLock) ;
						closeConnection((*it).first);
						pthread_mutex_lock(&connectionMapLock) ;
						//close((*it).first);
						//toBeClosed = (*it).first;
						//kill((*it).second.myId, SIGUSR2);
						//kill((*it).second.myReadId, SIGUSR2);
						//kill((*it).second.myWriteId, SIGUSR2);				
						//connectionMap.erase((*it).first);
						//it--;
					}
				}
			}
		}
		pthread_mutex_unlock(&connectionMapLock) ;
		//Auto-ShutDown Timer
		if(!inJoinNetwork)
		{
			if(myInfo->autoShutDown > 0)
				myInfo->autoShutDown--;
			else
			{
				if(!myInfo->autoShutDown)
				{
					kill(node_pid, SIGTERM);
				}
			}
		}


		// Status timer flag
		if (statusTimerFlag && !inJoinNetwork){
			--myInfo->statusResponseTimeout ;
			if (myInfo->statusResponseTimeout == 0){
				// Reset the status timer flag
				pthread_mutex_lock(&statusMsgLock) ;
				statusTimerFlag = 0 ;
				pthread_mutex_unlock(&statusMsgLock) ;

				// Write all the status responses in the output file
				writeToStatusFile() ;

			}

		}



	}
	pthread_exit(0);
	return 0;
}

