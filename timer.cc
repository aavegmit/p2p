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
	}
}
pthread_exit(0);
return 0;
}

