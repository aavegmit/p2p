#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "iniParser.h"
#include "main.h"

// Timer thread, decrements the value of timer for different timers
void *timer_thread(void *arg){

	while(!shutDown){

		sleep(1) ;

		if(shutDown)
		{
			//printf("Timer thread halted\n");
			break;
		}
		
		//JoinTimeOut, only accessible when node is Joining
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
					}
				}
			}
		}

		//KeepAliveTimeOut, monitors the value and then close the connection if the timeout becomes 0
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		if(!nodeConnectionMap.empty() && !inJoinNetwork)
		{
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
						connectionMap[(*it).second].keepAliveTimeOut=-1;
						pthread_mutex_unlock(&connectionMapLock) ;
						closeConnection((*it).second);
						//nodeConnectionMap.erase((*it).first);
						nodeConnectionMap.erase(it++);
						pthread_mutex_lock(&connectionMapLock) ;
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
					//sends the SIGTERM when needs to autoshutdown
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
				//writeToStatusFile() ;

				// Reset the status timer flag
				pthread_mutex_lock(&statusMsgLock) ;
				statusTimerFlag = 0 ;
				pthread_cond_signal(&statusMsgCV);
				pthread_mutex_unlock(&statusMsgLock) ;
			}
		}

		// CHECK timer flag
		if (checkTimerFlag && !inJoinNetwork){
			--myInfo->checkResponseTimeout ;
						//printf("Timer deducted %d\n", myInfo->checkResponseTimeout) ;
			if (myInfo->checkResponseTimeout <= 0){
				// Reset the status timer flag
				checkTimerFlag = 0 ;
				softRestartFlag = 1 ;


				// Write all the status responses in the output file
				//printf("No response from any beacon node. You are disconnected from the network. Connect again\n") ;
				pthread_mutex_lock(&nodeConnectionMapLock) ;
				for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
					notifyMessageSend((*it).second, 3);
				}
				pthread_mutex_unlock(&nodeConnectionMapLock) ;
				sleep(1);
				kill(node_pid, SIGTERM);
				//break;
				//				writeToStatusFile() ;

			}

		}

		//MsgLifetime timer, erases the message if the timer expires
		if(!inJoinNetwork)
		{
			pthread_mutex_lock(&MessageDBLock) ;
			//map<string, struct Packet>::iterator it_temp;
			map<string, struct Packet>::iterator tempIt ;
			for(map<string, struct Packet>::iterator it_temp = MessageDB.begin(); it_temp != MessageDB.end(); )
			{
				if((*it_temp).second.msgLifeTime > 0)
				{
					(*it_temp).second.msgLifeTime--;
					++it_temp;
					continue;
				}
				else
				{
					if((*it_temp).second.msgLifeTime == 0)
					{
						tempIt = it_temp ;
						++it_temp ;
						MessageDB.erase(tempIt);
					}
					else
						++it_temp;
				}
			}
			pthread_mutex_unlock(&MessageDBLock) ;
			
		}


		}
		pthread_exit(0);
		return 0;
	}

