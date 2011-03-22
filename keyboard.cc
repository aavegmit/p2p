#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"
#include <ctype.h>

using namespace std ;

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
        signal(SIGINT, my_handler);
        //alarm(myInfo->autoShutDown);
        
	while(!shutDown){
	sigaddset(&new_t, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);
        signal(SIGINT, my_handler);
	//memset(&tv, 0, sizeof(tv));
	//tv.tv_sec = 1;
	//tv.tv_usec = 0;
	printf("\nservant:%d> ", myInfo->portNo) ;
	if(shutDown)
		break;
	string inpS ;
	getline(cin, inpS) ;
	inp = const_cast<char *> (inpS.c_str()) ;
//	fgets((char *)inp, 511, stdin);
	
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
           
           
		if(!strcasecmp((char *)inp, "shutdown"))
		{
			sigset_t new_t;
			sigemptyset(&new_t);
			sigaddset(&new_t, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &new_t, NULL);

			shutDown = 1 ;

			//for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
				//pthread_mutex_lock(&connectionMapLock) ;
				//if(connectionMap[(*it).second].isReady == 2)	//closeConnection((*it).first);
					notifyMessageSend((*it).second, 1);
				//pthread_mutex_unlock(&connectionMapLock) ;
				//printf("Hi I am here\n");
			}
			pthread_mutex_unlock(&nodeConnectionMapLock) ;
			sleep(1);
			kill(node_pid, SIGTERM);
			break;
		}
		else if(strstr((char *)inp, "status neighbors")!=NULL)
		{
			
			sprintf((char *)inp, "%s ", inp);
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			unsigned char fileName[256];
			memset(&fileName, '\0', 256);
			for(int i=0;i<3;i++)
			{
				value = (unsigned char *)strtok(NULL, " ");
				if(value == NULL)
					break;
				if(i==0)
				{
					if(strcasecmp((char*)value, "neighbors")==0)
						continue;
					else
						break;
				}
				if(i==1) 
				{
					int digitCheckFlag = 0;
					for(int j=0;j<(int)strlen((char *)value);j++)
						if(isdigit(value[j]) == 0)
						{
							digitCheckFlag = 1;
							break;
						}
					if(digitCheckFlag)
						break;
					myInfo->status_ttl = (uint8_t)atoi((char *)value);
				}
				
				if(i==2)
				{
					strncpy((char *)myInfo->status_file, (char *)value, strlen((char *)value)) ;
					
					myInfo->statusResponseTimeout = myInfo->msgLifeTime + 2 ;
					//myInfo->status_ttl = 5 ;
					//strncpy((char *)myInfo->status_file, "status.out", 256) ;
					FILE *fp = fopen((char *)myInfo->status_file, "w") ;
					if (fp == NULL){
						fprintf(stderr,"File open failed\n") ;
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
					break;
				}
			}
			
		}

		memset(inp, '\0', 512) ;	
		cin.clear() ;
		fflush(stdin);
	}
	pthread_exit(0);
	return 0;
}

