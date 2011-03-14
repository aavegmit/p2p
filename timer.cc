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
	
	//JoinTimeOut
	if(!myInfo->isBeacon)
	{
		if(myInfo->joinTimeOut > 0)
			myInfo->joinTimeOut--;
		else
		{
			if(myInfo->joinTimeOut==0)
			{
				printf("Timer out..: %d\n", (int)pthread_self()) ;
				kill(accept_pid, SIGUSR1);
				//myInfo->joinTimeOut--;
			}
		}
	}
	
	//KeepAliveTimeOut
	for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it){
	
		if((*it).second.keepAliveTimeOut > 0)
			(*it).second.keepAliveTimeOut--;
		else
		{
			/*struct Message mes;
			mes.type = 0xf8;
			printf("Sent Keep Alive Message to: %d\n", (*it).first);
			pushMessageinQ((*it).first, mes);*/
			printf("Erasing entry from Map for : %d\n", (*it).first);
			closeConnection((*it).first);
			connectionMap.erase((*it).first);
		}
	}	


}

return 0;
}

