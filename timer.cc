#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "iniParser.h"
#include "main.h"
//#include <thread.h>


void *timer_thread(void *arg){

	while(!shutDown){

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
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		if(!nodeConnectionMap.empty() && !inJoinNetwork)
		{
			//printf("My Map size is: %d\n", (int)nodeConnectionMap.size());
			for (map<struct node, int >::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ){
			//printf("Values in my mAP are: %s, %d\n", (*it).first.hostname, (*it).first.portNo);
			pthread_mutex_lock(&connectionMapLock) ;
				if(connectionMap[(*it).second].keepAliveTimeOut > 0)
				{
					connectionMap[(*it).second].keepAliveTimeOut--;
					it++;
				}
				else
				{
					if(connectionMap[(*it).second].keepAliveTimeOut == 0)
					{
						//printf("Erasing entry from Map for : %d, %d\n", (*it).first, connectionMap[(*it).first].keepAliveTimeOut);
						connectionMap[(*it).second].keepAliveTimeOut=-1;
						pthread_mutex_unlock(&connectionMapLock) ;
						closeConnection((*it).second);
						//nodeConnectionMap.erase((*it).first);
						nodeConnectionMap.erase(it++);
						pthread_mutex_lock(&connectionMapLock) ;
						//close((*it).first);
						//toBeClosed = (*it).first;
						//kill((*it).second.myId, SIGUSR2);
						//kill((*it).second.myReadId, SIGUSR2);
						//kill((*it).second.myWriteId, SIGUSR2);				
						//connectionMap.erase((*it).first);
						//it--;
					}
					else
						it++;
					
				}
			pthread_mutex_unlock(&connectionMapLock) ;
			}
		}
		pthread_mutex_unlock(&nodeConnectionMapLock) ;

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
			if(myInfo->statusResponseTimeout > 0)
				--myInfo->statusResponseTimeout ;
			else //if (myInfo->statusResponseTimeout == 0){
			{
				// Write all the status responses in the output file
				writeToStatusFile() ;
				
				// Reset the status timer flag
				pthread_mutex_lock(&statusMsgLock) ;
				statusTimerFlag = 0 ;
				pthread_cond_signal(&statusMsgCV);
				pthread_mutex_unlock(&statusMsgLock) ;
				
			}
		}
		
		//MsgLifetime timer
		if(!inJoinNetwork)
		{
			pthread_mutex_lock(&MessageDBLock) ;
			map<string, struct Packet>::iterator it_temp;
			for(map<string, struct Packet>::iterator it = MessageDB.begin(); it != MessageDB.end(); )
			{
				if((*it).second.msgLifeTime > 0)
				{
					(*it).second.msgLifeTime--;
					++it;
					continue;
				}
				else
				{
					if((*it).second.msgLifeTime == 0)
					{
						//printf("This entry has been erased\n");
						//it_temp = it;
						//it++;	
						MessageDB.erase((it)++);
					}
					else
						++it;
				}
			}
			pthread_mutex_unlock(&MessageDBLock) ;
		}


	}
	pthread_exit(0);
	return 0;
}

