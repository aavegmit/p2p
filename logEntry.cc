#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "iniParser.h"
#include "main.h"

unsigned char data[256];
unsigned char finalData[512];
unsigned char msg_type[4];
struct node n;
void getNeighbor(int sockfd)
{
	//struct node *n = (struct node *)malloc(sizeof(struct node));
	//memset(&n, NULL, sizeof(n));
	pthread_mutex_lock(&nodeConnectionMapLock);
	for(map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end() ; ++it){
		if((*it).second == sockfd)
		{
			n = (*it).first;
			break;
		}
	}
	pthread_mutex_unlock(&nodeConnectionMapLock);	
}

unsigned char* messageType(uint8_t message_type)
{
	//char *msg_type = (char *)malloc(sizeof(char)*4);
	//char msg_type[4];
	memset(&msg_type, '\0', 4);
	const char *temp_msg_type;

	switch(message_type)
	{

		case 0xfc : 	//strncpy((char *)msg_type, "JNRQ", 4);
			temp_msg_type = "JNRQ";
			break;
		case 0xfb : 	//strncpy((char *)msg_type, "JNRS", 4);
			temp_msg_type ="JNRS";
			break; 
		case 0xfa : 	//strncpy((char *)msg_type, "HLLO", 4);
			temp_msg_type ="HLLO";
			break;
		case 0xf8 : 	//strncpy((char *)msg_type, "KPAV", 4);
			temp_msg_type ="KPAV";
			break;
		case 0xf7 : 	//strncpy((char *)msg_type, "NTFY", 4);
			temp_msg_type ="NTFY";
			break;
		case 0xf6 : 	//strncpy((char *)msg_type, "CKRQ", 4);
			temp_msg_type ="CKRQ";
			break;
		case 0xf5 : 	//strncpy((char *)msg_type, "CKRS", 4);
			temp_msg_type ="CKRS";
			break;
		case 0xac : 	//strncpy((char *)msg_type, "STRQ", 4);
			temp_msg_type ="STRQ";
			break;
		case 0xab : 	//strncpy((char *)msg_type, "STRS", 4);
			temp_msg_type ="STRS";
			break;
		default : break;
	}
	for(unsigned int i=0;i<4;i++)
		msg_type[i]=temp_msg_type[i];
	msg_type[4] = '\0';
	return msg_type;
}

void dataFromBuffer(uint8_t message_type, unsigned char *buffer)
{
	//unsigned char portNo[2];
	int portNo;
	memset(&portNo, '\0', sizeof(portNo));
	unsigned char uoid[4];
	memset(&uoid, '\0', sizeof(uoid));
	long int distance;
	memset(&distance, '\0', sizeof(distance));
	unsigned char hostName[256];
	memset(&hostName, '\0', sizeof(hostName));
	int errorCode  = 0;
	int statusType = 0;

	switch(message_type)
	{
		case 0xfc : 	//Join Request
			memcpy(&portNo, buffer+4, 2) ;
			for(unsigned int i=0;i < strlen((char *)buffer)-6;i++)
				hostName[i] = buffer[i+6];
			//strncpy((char *)hostName, (char *)buffer+2, strlen((char *)buffer)-2) ;
			sprintf((char *)data, "%d %s", portNo, (char *)hostName);
			break;

		case 0xfb : 	//Join Response
			//memcpy(uoid, &buffer[17], 4);
			for(unsigned int i=0;i < 4;i++)
				uoid[i] = buffer[17+i];
			memcpy(&distance, &buffer[20], 4) ;
			memcpy(&portNo, &buffer[24], 2) ;
			for(unsigned int i=26;buffer[i]!='\0';i++)
				hostName[i-26] = buffer[i];
			//strncpy((char *)hostName, ((char *)buffer+26), strlen((char *)buffer)-26) ;
			sprintf((char *)data, "%02x%02x%02x%02x %ld %d %s", uoid[0], uoid[1], uoid[2], uoid[3], distance, portNo, hostName);
			break; 

		case 0xfa : 	//Hello Message
			memcpy(&portNo, buffer, 2) ;
			//strncpy((char *)hostName, (char *)buffer+2, strlen((char *)buffer)-2) ;
			for(unsigned int i=0;i < strlen((char *)buffer)-2;i++)
				hostName[i] = buffer[i+2];
			sprintf((char *)data, "%d %s", portNo, (char *)hostName);
			break;

		case 0xf8 : 	//msg_type = "KPAV"
			break;
		case 0xf7 : 	//msg_type = "NTFY"
			memcpy(&errorCode, buffer, 1);
			sprintf((char *)data, "%d", errorCode);
			break;
		case 0xf6 : 	//msg_type = "CKRQ"
			break;
		case 0xf5 : 	//msg_type = "CKRS"
			break;
		case 0xac : 	//msg_type = "STRQ"
			memcpy(&statusType, buffer, 1) ;
			sprintf((char *)data, "%d", statusType);				
			break;
		case 0xab : 	//msg_type = "STRS"
			break;
	}
}

//Creates the string which writes into the log file

unsigned char *createLogEntry(unsigned char mode, int sockfd, unsigned char header[], unsigned char *buffer)
{

	uint8_t message_type=0;
	uint8_t ttl=0;
	uint32_t data_len=0;
	unsigned char uoid[4];
	unsigned char msg_type[4];
	memset(&msg_type, '\0', 4);
	memset(&uoid, '\0', 4);
	unsigned char hostname[256];
	memset(&hostname, '\0', sizeof(hostname));
	memset(&data, '\0', sizeof(data));
	memset(&finalData, '\0', sizeof(finalData));
	struct timeval tv;
	memset(&tv, NULL, sizeof(tv));
	//struct node *n;
	memset(&n, NULL, sizeof(n));

	gettimeofday(&tv, NULL);

	memcpy(&message_type, header, 1);
	memcpy(uoid,       header+17, 4);
	memcpy(&ttl,       header+21, 1);
	memcpy(&data_len,  header+23, 4);

	getNeighbor(sockfd);
	/*gethostname((char *)hostname, 256);
	  hostname[255] = '\0';*/


	//if(n == NULL)
	//	return NULL;
	if(!messageType(message_type))
		return NULL;

	//strncpy((char *)msg_type, (char *)messageType(message_type), 4);
	unsigned char *tempChar = messageType(message_type);
	for(unsigned  int i=0;i < 4;i++)
		msg_type[i] = tempChar[i];
	msg_type[4]='\0';


	dataFromBuffer(message_type, buffer);

	if(data == NULL)
		return NULL;

	if(mode == 'r')
	{
		// logging for read mode
		sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
//		printf("Final DATA is; %s\n", finalData);
		fflush(f_log);
	}
	else if(mode == 's')
	{
		//log for messages sent
		sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
//		printf("Final DATA is; %s\n", finalData);	
	}
	else
	{
		// log for messages forwarded
		sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
//		printf("Final DATA is; %s\n", finalData);	
	}

	return finalData;
}

void writeLogEntry(unsigned char *tobewrittendata)
{
	fprintf(f_log, "%s", tobewrittendata);
}
