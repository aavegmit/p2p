#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"

void *keyboard_thread(void *arg){
	fd_set rfds;
	//struct timeval tv;
	char *inp = new char[512] ;
	memset(inp, '\0', 512) ;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
        signal(SIGUSR2, my_handler);
        //alarm(myInfo->autoShutDown);
        
	while(!shutDown){
	//memset(&tv, 0, sizeof(tv));
	//tv.tv_sec = 1;
	//tv.tv_usec = 0;
	printf("servant:%d> ", myInfo->portNo) ;
	if(shutDown)
		break;
	fgets(inp, 511, stdin);
	
	if(shutDown)
		break;
	
			
/*			
	int retval = select(1, &rfds, NULL, NULL, &tv);
  

           if (retval == -1)
               perror("select()");
           else 
           {
                if (retval)
        	   {
	        	   printf("servant:%d> ", myInfo->portNo) ;
        	   	fgets(inp, 511, stdin);
		   }
  
        	   else
        	   {
        	   	if(shutDown)
	        	       	break;
        	   }
           }
*/           
           
           
		if(!strcasecmp(inp, "shutdown\n"))
		{
			sigset_t new_t;
			sigemptyset(&new_t);
			sigaddset(&new_t, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &new_t, NULL);

			shutDown = 1 ;

			//for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
				pthread_mutex_lock(&connectionMapLock) ;
				if(connectionMap[(*it).second].isReady == 2)	//closeConnection((*it).first);
					notifyMessageSend((*it).second, 1);
				pthread_mutex_unlock(&connectionMapLock) ;
				//printf("Hi I am here\n");
			}
			pthread_mutex_unlock(&nodeConnectionMapLock) ;

			kill(node_pid, SIGTERM);
			break;
		}
		else if(!strcasecmp(inp, "status\n"))
		{
			statusTimerFlag = 1 ;
			myInfo->statusResponseTimeout = myInfo->msgLifeTime + 2 ;
			myInfo->status_ttl = 5 ;
			strncpy(myInfo->status_file, "status.out", 256) ;
			FILE *fp = fopen(myInfo->status_file, "w") ;
			if (fp == NULL){
				fprintf(stderr,"File open failed\n") ;
				exit(0) ;
			}
			fputs("V -t * -v 1.0a5\n", fp) ;
			fclose(fp) ;
			getStatus() ;
		}

		memset(inp, '\0', 512) ;	
//		fflush(stdin);
	}
	pthread_exit(0);
	return 0;

}

