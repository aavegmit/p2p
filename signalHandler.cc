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
		//exit(1);
	}
}

