
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include<signal.h>

#include "iniParser.h"
#include "main.h"


void *timer_thread(void *arg){

while(1){

	sleep(1) ;
	printf("Timer out..\n") ;
	
	//JoinTimeOut
	if(myInfo->joinTimeOut > 0)
		myInfo->joinTimeOut--;
	else
	{
		if(myInfo->joinTimeOut==0)
		{
			kill(accept_pid, SIGUSR1);
			myInfo->joinTimeOut--;
		}
	}
	
	//KeepAliveTimeOut
	


}

return 0;
}

