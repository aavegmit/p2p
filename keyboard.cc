#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"
#include <ctype.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

using namespace std ;

//This thread is a keborad thread, which tajes input from user, such as
// shutdown and STATUS NEIGHBORS.....
// handling of SIGINT
void *keyboard_thread(void *arg){
	fd_set rfds;
	//struct timeval tv;
	//char *inp = new char[512] ;
	char *inp;
	inp = (char *)malloc(512 * sizeof(char)) ;
	memset(inp, '\0', 512) ;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
        signal(SIGUSR2, my_handler);
	sigset_t new_t ;
	sigemptyset(&new_t);
	sigaddset(&new_t, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);
        //signal(SIGINT, my_handler);
        //alarm(myInfo->autoShutDown);
        
	while(!shutDown){
	/*pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);*/
        signal(SIGINT, my_handler);
	printf("\nservant:%d> ", myInfo->portNo) ;
	if(shutDown)
		break;
	//string inpS ;
	//getline(cin, inpS) ;
	//inp = const_cast<char *> (inpS.c_str()) ;
	fgets((char *)inp, 511, stdin);
	
	if(shutDown)
		break;
	
			
		//User enters the 'shutdown' command, nodes shutdown after sending the notify message to it's neighbors
		if(!strcasecmp((char *)inp, "shutdown\n"))
		{
			/*sigset_t new_t;
			sigemptyset(&new_t);
			sigaddset(&new_t, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &new_t, NULL);*/

			shutDown = 1 ;

			//for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
					notifyMessageSend((*it).second, 1);
			}
			pthread_mutex_unlock(&nodeConnectionMapLock) ;
			sleep(1);
			//sending the SIGTERM signal to the main thread, for shutdown
			kill(node_pid, SIGTERM);
			break;
		}
		//user enters the status neighbors command, this inittates the flooding of status message
		// creates the status file, nam file and shows the network
		else if(strstr((char *)inp, "status neighbors")!=NULL)
		{
			
			//fprintf(stdin, "%s ", inp);
			
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			value = (unsigned char *)strtok(NULL, " ");
			
			unsigned char fileName[256];
			memset(&fileName, '\0', 256);

			//gets the ttl value here
			value = (unsigned char *)strtok(NULL, " ");
			if(value ==NULL)
				continue;
			//check if the entered value id digit or not
			for(int j=0;j<(int)strlen((char *)value);j++)
				if(isdigit(value[j]) == 0)
					continue;
			myInfo->status_ttl = (uint8_t)atoi((char *)value);

			//name of the status file
			value = (unsigned char *)strtok(NULL, "\n");
			if(value ==NULL)
				continue;					
			strncpy((char *)myInfo->status_file, (char *)value, strlen((char *)value)) ;
			myInfo->status_file[strlen((char *)myInfo->status_file)] = '\0';
				
			myInfo->statusResponseTimeout = myInfo->msgLifeTime + 10 ;
			FILE *fp = fopen((char *)myInfo->status_file, "w") ;
			if (fp == NULL){
				//fprintf(stderr,"File open failed\n") ;
				writeLogEntry((unsigned char *)"In Keyborad thread: Status File open failed\n");
				exit(0) ;
			}
			fputs("V -t * -v 1.0a5\n", fp) ;
			fclose(fp) ;
			getStatus() ;
					
			// waiting for status time out, so that command prompt can be returned
			pthread_mutex_lock(&statusMsgLock) ;
			statusTimerFlag = 1 ;
			pthread_cond_wait(&statusMsgCV, &statusMsgLock);
			pthread_mutex_unlock(&statusMsgLock) ;
			//printf("WOW!!!! %d\n", myInfo->statusResponseTimeout);
			writeToStatusFile() ;
		}
		

		memset(inp, '\0', 512) ;	
		//cin.clear() ;
		fflush(stdin);
	}
	
	//printf("KeyBoardThread Halted\n");
	pthread_exit(0);
	return 0;
}

