#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"

void *keyboard_thread(void *arg){
	char *inp = new char[512] ;
	memset(inp, '\0', 512) ;
	while(1){
		printf("servant:%d> ", myInfo->portNo) ;

		fgets(inp, 511, stdin) ;

		if(!strcasecmp(inp, "shutdown\n"))
		{
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
		else
		{
			printf("Hi!!!!!!!\n");
		}

		memset(inp, '\0', 512) ;	
	}
	pthread_exit(0);
	return 0;

}

