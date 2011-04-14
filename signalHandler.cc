#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<signal.h>

#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"
/* Signal Handler */
void my_handler(int nSig)
{
	//Checks the signal
	if (nSig == SIGUSR1) {
		//printf("Signal Handling Success!!!!!\n");
		//Indicates the Join timeout
		if(myInfo->joinTimeOut == 0)
		{
			joinTimeOutFlag = 1;
			
			closeConnection( (*nodeConnectionMap.begin()).second  ) ;
			myInfo->joinTimeOut--;
		}
//		if(shutDown)
			
		//pthread_exit(0);
	}

	if (nSig == SIGUSR2) {
		//closeConnection(toBeClosed);
		//pthread_exit(0);
	}
	//signal for shutdown
	if(nSig == SIGTERM)
	{
		shutDown = 1;
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it)
			closeConnection((*it).second);
		//nodeConnectionMap.clear();
		pthread_mutex_unlock(&nodeConnectionMapLock) ;
		
		shutdown(nSocket_accept, SHUT_RDWR);
		close(nSocket_accept);
		pthread_kill(k_thread, SIGUSR2);
		
		if(myInfo->isBeacon)
		{
			for (map<pthread_t, bool>::iterator it = myConnectThread.begin(); it != myConnectThread.end(); ++it)
			{
				if((*it).second)
					pthread_kill((*it).first, SIGUSR2);
			}
		}
		
	}
	if(nSig == SIGALRM)
	{
		pthread_exit(0);
	}
	//signal handling of ctrl+c
	if(nSig == SIGINT)
	{
		pthread_mutex_lock(&statusMsgLock) ;
		if(statusTimerFlag)
			myInfo->statusResponseTimeout = 0;
		pthread_mutex_unlock(&statusMsgLock) ;
		pthread_mutex_lock(&searchMsgLock) ;
		searchTimerFlag = 0 ;
		pthread_cond_signal(&searchMsgCV) ;
		pthread_mutex_unlock(&searchMsgLock) ;
	}

}

