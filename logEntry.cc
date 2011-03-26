#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "iniParser.h"
#include "main.h"

unsigned char data[256];
unsigned char finalData[512];
unsigned char msg_type[5];
struct node n;
uint32_t data_len=0;

//gets the neighbors hostname and port no for logging purposes
void getNeighbor(int sockfd)
{
	pthread_mutex_lock(&connectionMapLock);
	n = connectionMap[sockfd].n;
	pthread_mutex_unlock(&connectionMapLock);
}

//determinse the type of message on the basis of the messg_type code
void messageType(uint8_t message_type)
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
}

//setting the data filed for logging, extarcting the values from the buffer
void dataFromBuffer(uint8_t message_type, unsigned char *buffer)
{
//unsigned char portNo[2];
unsigned short int portNo;
memset(&portNo, '\0', sizeof(portNo));
unsigned char uoid[4];
memset(&uoid, '\0', sizeof(uoid));
uint32_t distance;
memset(&distance, '\0', sizeof(distance));
unsigned char hostName[256];
memset(&hostName, '\0', sizeof(hostName));
uint8_t errorCode  = 0;
uint8_t statusType = 0x00;

	switch(message_type)
	{
		case 0xfc : 	//Join Request
			memcpy(&portNo, buffer+4, 2) ;
			for(unsigned int i=0;i < data_len-6;i++)
				hostName[i] = buffer[i+6];
			//strncpy((char *)hostName, (char *)buffer+2, strlen((char *)buffer)-2) ;
			sprintf((char *)data, "%d %s", portNo, (char *)hostName);
			break;

		case 0xfb : 	//Join Response
			//memcpy(uoid, &buffer[17], 4);
			for(unsigned int i=0;i < 4;i++)
				uoid[i] = buffer[16+i];
			memcpy(&distance, &buffer[20], 4) ;
			memcpy(&portNo, &buffer[24], 2) ;
			for(unsigned int i=26;i < data_len;i++)
				hostName[i-26] = buffer[i];
			//strncpy((char *)hostName, ((char *)buffer+26), strlen((char *)buffer)-26) ;
			sprintf((char *)data, "%02x%02x%02x%02x %d %d %s", uoid[0], uoid[1], uoid[2], uoid[3], distance, portNo, hostName);
			break; 

		case 0xfa : 	//Hello Message
			memcpy(&portNo, buffer, 2) ;
			//strncpy((char *)hostName, (char *)buffer+2, strlen((char *)buffer)-2) ;
			for(unsigned int i=0;i < data_len-2;i++)
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
			for(unsigned int i=0;i < 4;i++)
				uoid[i] = buffer[16+i];
			sprintf((char *)data, "%02x%02x%02x%02x", uoid[0], uoid[1], uoid[2], uoid[3]);
			break;
		case 0xac : 	//msg_type = "STRQ"
			memcpy(&statusType, buffer, 1) ;
			if(statusType == 0x01)
				sprintf((char *)data, "%s", "neighbors");
			else
				sprintf((char *)data, "%s", "files");
			break;
		case 0xab : 	//msg_type = "STRS"
			for(unsigned int i=0;i < 4;i++)
				uoid[i] = buffer[16+i];
			sprintf((char *)data, "%02x%02x%02x%02x", uoid[0], uoid[1], uoid[2], uoid[3]);
			break;
	}
}

//Creates the string which writes into the log file

unsigned char *createLogEntry(unsigned char mode, int sockfd, unsigned char header[], unsigned char *buffer)
{

uint8_t message_type=0;
uint8_t ttl=0;
unsigned char uoid[4];
memset(&msg_type, '\0', 4);
memset(&uoid, '\0', 4);
unsigned char hostname[256];
memset(&hostname, '\0', sizeof(hostname));
memset(&data, '\0', sizeof(data));
memset(&finalData, '\0', sizeof(finalData));
struct timeval tv;
memset(&tv, NULL, sizeof(tv));
//struct node *n;
memset(&n, 0, sizeof(n));

gettimeofday(&tv, NULL);

memcpy(&message_type, header, 1);
//memcpy(uoid,       header+17, 4);
for(int i=0;i<4;i++)
	uoid[i] = header[17+i];
memcpy(&ttl,       header+21, 1);
memcpy(&data_len,  header+23, 4);


//determins the code of message_type
messageType(message_type);

//gets the hostname and port no of neighbor
getNeighbor(sockfd);

//set the data filed on logging
dataFromBuffer(message_type, buffer);

if(data == NULL)
	return NULL;
	
if(mode == 'r')
{
// logging for read mode
	sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
fflush(f_log);
}
else if(mode == 's')
{
//log for messages sent
	sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
fflush(f_log);
}
else
{
// log for messages forwarded
	sprintf((char *)finalData, "%c %10ld.%03d %s_%d %s %d %d %02x%02x%02x%02x %s\n", mode, tv.tv_sec, (int)(tv.tv_usec/1000), (char *)n.hostname, n.portNo, (char *)msg_type, (data_len + HEADER_SIZE),  ttl, uoid[0], uoid[1], uoid[2], uoid[3], (char *)data);
fflush(f_log);
}

	return finalData;
}

//this functions writes the passed string into the log entry
void writeLogEntry(unsigned char *tobewrittendata)
{
	fprintf(f_log, "%s", tobewrittendata);
	fflush(f_log);
}
