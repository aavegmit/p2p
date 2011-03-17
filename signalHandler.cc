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
	/* Signal for the Timeout */
/*	if (nSig == SIGALRM) {
	printf("Timeout occurred...Closing conncetion\n");
		close(nSocket);
		toBeExit = 1;
	}
*/	/* Signal whle pressing Ctrl + C */
/*	else if(nSig == SIGINT)
	{
		printf("You Pressed Ctrl + C\n");
		close(nSocket);
		toBeExit = 1;
	} 
*/	/* Signal raised by Parent for Child */
	if (nSig == SIGUSR1) {
		printf("Signal Handling Success!!!!!\n");
		if(myInfo->joinTimeOut == 0)
		{
			joinTimeOutFlag = 1;
			myInfo->joinTimeOut--;
		}
		if(shutDown)
			printf("I am awake!!!!\n");
		//pthread_exit(0);
	}

	if (nSig == SIGUSR2) {
		printf("Signal Handling Success: %d\n", (int)pthread_self());
		//closeConnection(toBeClosed);
		pthread_exit(0);
	}
	if(nSig == SIGTERM)
	{
		//printf("Signal Handling SIGTERM %d\n", (int)pthread_self());
		shutDown = 1;
		for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			closeConnection((*it).first);
		shutdown(nSocket_accept, SHUT_RDWR);
		close(nSocket_accept);
		pthread_kill(k_thread, SIGUSR2);
		//fprintf(stdout, NULL) ;
		//pthread_mutex_lock(&connectionMapLock) ;
		//pthread_mutex_unlock(&connectionMapLock) ;
		//Signaling it's child
		
	}
	if(nSig == SIGALRM)
	{
		//printf("OUT OF FGETS\n");
		pthread_exit(0);
	}
}

