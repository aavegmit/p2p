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
		else if(!strcasecmp(inp, "status neighbors\n"))
		{
			printf("Under construction....\n");
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

