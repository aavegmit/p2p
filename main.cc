#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <signal.h>
#include "main.h"
#include "iniParser.h"

int resetFlag = 0;

int usage()
{
	printf("Usage is: \"sv_node [-reset] <.ini file>\"\n");
	return false;
}

int processCommandLine(int argc, char *argv[])
{
	int iniFlag=0;

	if(argc<2 || argc > 3)
		return usage();

	for(int i=1;i<argc;i++)
	{
		argv++;
		if(*argv[0]=='-')
		{
			if(strcmp(*argv,"-reset")!=0)
				return usage();
			else
				resetFlag = 1;
		}
		else
		{
			if(strstr(*argv, ".ini")==NULL)
				return usage();
			else
				iniFlag = 1;
		}
	}
	if(iniFlag!=1)
		return usage();
	else
		return true;
}


int main(int argc, char *argv[])
{
	if(!processCommandLine(argc, argv))
		exit(0);

	// Call INI parser here - returns a structure

	// Populate the structure manually for testing
	//
	//
	//
	// -------------------------------------------

	// Call the Keyboard thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t k_thread ;
	void *thread_result ;
	int res ;
	res = pthread_create(&k_thread, NULL, keyboard_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	// Call the timer thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t t_thread ;
	res = pthread_create(&t_thread, NULL, timer_thread , (void *)NULL);
	if (res != 0) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}


	// Thread Join code taken from WROX Publications
		res = pthread_join(k_thread, &thread_result);
	if (res != 0) {
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}



	return 0;
}
