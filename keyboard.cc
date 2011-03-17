#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"

void *keyboard_thread(void *arg){
	fd_set rfds;
	struct timeval tv;
	char *inp = new char[512] ;
	memset(inp, '\0', 512) ;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
        signal(SIGUSR2, my_handler);
        //alarm(myInfo->autoShutDown);
        
	while(1){
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	tv.tv_usec = 0;
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

			for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			{
				if((*it).second.isReady == 2)	//closeConnection((*it).first);
					notifyMessageSend((*it).first, 1);
			}

			kill(node_pid, SIGTERM);
			break;
		}
		else if(!strcasecmp(inp, "status neighbors\n"))
		{
			printf("Under construction....\n");
		}

		memset(inp, '\0', 512) ;	
	}
pthread_exit(0);
	return 0;

}

