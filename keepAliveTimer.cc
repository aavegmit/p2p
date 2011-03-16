#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<map>

#include "main.h"
#include "keepAliveTimer.h"

void *keepAliveTimer_thread(void *arg){

while(1)
{
	sleep(1);

	if(shutDown)
	{
		printf("KeepAlive thread halted\n");
		break;
	}
	for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it){
	
		if((*it).second.keepAliveTimer > 0)
			(*it).second.keepAliveTimer--;
		else
		{
			if((*it).second.keepAliveTimeOut!=-1 && (*it).second.isReady==2)
			{
				struct Message mes;
				mes.type = 0xf8;
				//printf("Sent Keep Alive Message to: %d\n", (*it).first);
				pushMessageinQ((*it).first, mes);
			}
		}
	}
}
pthread_exit(0);
return 0;
}

