#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "iniParser.h"
#include "signalHandler.h"
#include "indexSearch.h"
#include <ctype.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "indexSearch.h"

using namespace std ;

//This thread is a keborad thread, which tajes input from user, such as
// shutdown and STATUS NEIGHBORS.....
// handling of SIGINT
void *keyboard_thread(void *arg){
	fd_set rfds;
	//struct timeval tv;
	//char *inp = new char[512] ;
	bool checkFlag = 0;
	char *inp;
	inp = (char *)malloc(1024 * sizeof(char)) ;
	memset(inp, '\0', 1024) ;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
        signal(SIGUSR2, my_handler);
	sigset_t new_t ;
	sigemptyset(&new_t);
	sigaddset(&new_t, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);
        //signal(SIGINT, my_handler);
        //alarm(myInfo->autoShutDown);
        
	while(!shutDown){
	/*pthread_sigmask(SIG_UNBLOCK, &new_t, NULL);*/
        signal(SIGINT, my_handler);
	printf("\nservant:%d> ", myInfo->portNo) ;
	if(shutDown)
		break;
	//string inpS ;
	//getline(cin, inpS) ;
	//inp = const_cast<char *> (inpS.c_str()) ;
	fgets((char *)inp, 511, stdin);
	
	if(shutDown)
		break;
	
			
		//User enters the 'shutdown' command, nodes shutdown after sending the notify message to it's neighbors
		if(!strcasecmp((char *)inp, "shutdown\n"))
		{
			/*sigset_t new_t;
			sigemptyset(&new_t);
			sigaddset(&new_t, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &new_t, NULL);*/

			shutDown = 1 ;

			//for (map<int, struct connectionNode>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it)
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
					notifyMessageSend((*it).second, 1);
			}
			pthread_mutex_unlock(&nodeConnectionMapLock) ;
			sleep(1);
			//sending the SIGTERM signal to the main thread, for shutdown
			kill(node_pid, SIGTERM);
			break;
		}
		//user enters the status neighbors command, this inittates the flooding of status message
		// creates the status file, nam file and shows the network
		else if(strstr((char *)inp, "status neighbors")!=NULL)
		{
			
			//fprintf(stdin, "%s ", inp);
			checkFlag = 0;
			
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			value = (unsigned char *)strtok(NULL, " ");
			
			unsigned char fileName[256];
			memset(&fileName, '\0', 256);

			//gets the ttl value here
			value = (unsigned char *)strtok(NULL, " ");
			if(value ==NULL)
				continue;
			//check if the entered value id digit or not
			for(int j=0;j<(int)strlen((char *)value);j++)
				if(isdigit(value[j]) == 0)
					continue;
			myInfo->status_ttl = (uint8_t)atoi((char *)value);

			//name of the status file
			value = (unsigned char *)strtok(NULL, "\n");
			if(value ==NULL)
				continue;					
			strncpy((char *)myInfo->status_file, (char *)value, strlen((char *)value)) ;
			myInfo->status_file[strlen((char *)myInfo->status_file)] = '\0';
				
			myInfo->statusResponseTimeout = myInfo->msgLifeTime + 10 ;
			FILE *fp = fopen((char *)myInfo->status_file, "w") ;
			if (fp == NULL){
				//fprintf(stderr,"File open failed\n") ;
				writeLogEntry((unsigned char *)"In Keyborad thread: Status File open failed\n");
				exit(0) ;
			}
			fputs("V -t * -v 1.0a5\n", fp) ;
			fclose(fp) ;
			getStatus() ;
					
			// waiting for status time out, so that command prompt can be returned
			pthread_mutex_lock(&statusMsgLock) ;
			statusTimerFlag = 1 ;
			pthread_cond_wait(&statusMsgCV, &statusMsgLock);
			pthread_mutex_unlock(&statusMsgLock) ;
			//printf("WOW!!!! %d\n", myInfo->statusResponseTimeout);
			writeToStatusFile() ;
		}
		else if(strstr((char *)inp, "status files")!=NULL)
		{
			
			//fprintf(stdin, "%s ", inp);
			checkFlag = 0;
			
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			value = (unsigned char *)strtok(NULL, " ");
			
			unsigned char fileName[256];
			memset(&fileName, '\0', 256);

			//gets the ttl value here
			value = (unsigned char *)strtok(NULL, " ");
			if(value ==NULL)
				continue;
			//check if the entered value id digit or not
			for(int j=0;j<(int)strlen((char *)value);j++)
				if(isdigit(value[j]) == 0)
					continue;
			myInfo->status_ttl = (uint8_t)atoi((char *)value);

			//name of the status file
			value = (unsigned char *)strtok(NULL, "\n");
			if(value ==NULL)
				continue;					
			strncpy((char *)myInfo->status_file, (char *)value, strlen((char *)value)) ;
			myInfo->status_file[strlen((char *)myInfo->status_file)] = '\0';
				
			myInfo->statusResponseTimeout = myInfo->msgLifeTime + 10 ;
			FILE *fp = fopen((char *)myInfo->status_file, "w") ;
			if (fp == NULL){
				//fprintf(stderr,"File open failed\n") ;
				writeLogEntry((unsigned char *)"In Keyborad thread: Status File open failed\n");
				exit(0) ;
			}
			fclose(fp) ;
			getStatusTypeFiles() ;
					
			// waiting for status time out, so that command prompt can be returned
			pthread_mutex_lock(&statusMsgLock) ;
			statusTimerFlag = 1 ;
			pthread_cond_wait(&statusMsgCV, &statusMsgLock);
			pthread_mutex_unlock(&statusMsgLock) ;
			//printf("WOW!!!! %d\n", myInfo->statusResponseTimeout);
			writeToStatusFile_TypeFiles() ;
		}
		else if(strncasecmp((char *)inp, "store ", 6) == 0)
		{
			//inp[strlen((char *)inp)-1] = '\0';
			checkFlag = 0;
			
			struct metaData metadata;
			unsigned char *key;
			memset(&metadata, 0, sizeof(metadata));
			metadata.keywords =  new list<string >();
						
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			value = (unsigned char *)strtok(NULL, " ");
			strncpy((char *)metadata.fileName, (char *)value, strlen((char *)value));//file Name stored
			
			//fileSize needed to be stored
			struct stat st;
			stat((char *)metadata.fileName, &st);
			metadata.fileSize = (long int)st.st_size;
		
			//sha1 value needed to be stored
			unsigned char buffer;
			//memset(buffer, '\0', 256);
			FILE *f = fopen((char *)metadata.fileName, "rb");
			if(f==NULL)
			{
				printf("File Does not exist!!!\n");
				exit(0);
			}
			SHA_CTX *c = (SHA_CTX *)malloc(sizeof(SHA_CTX));
			//memset(c, 0, sizeof(c));
	      		memset(c, 0, sizeof(c));
	      		
			SHA1_Init(c);
			while(fread(&buffer,1,1,f)!=0)
			{
			        SHA1_Update(c, &buffer, 1);
			}
			SHA1_Final(metadata.sha1, c);


			free(c);
			fclose(f);

			/*unsigned char password[SHA_DIGEST_LENGTH] ;
			GetUOID( const_cast<char *> ("password"), password, sizeof(password)) ;
			strncpy((char *)metadata.nonce, (char *)toSHA1(password), 20);*/
			
			
			value = (unsigned char *)strtok(NULL, " "); //ttl value
			for(int i=0;i<strlen((char *)value);i++)
				if(isdigit(value[i]) == 0)
					continue;
			
			value = (unsigned char *)strtok(NULL, "\n");
			//unsigned char *temp_temp_value = (unsigned char *)strtok((char *)value, "?");
			//printf("Vlaue is: %s\n", value);
			
			/*unsigned char temp_value[strlen((char *)value)];
			memset(temp_value, '\0', sizeof(temp_value));
			sprintf((char *)temp_value, "%s", (char *)value);*/
			//while(sscanf((char *)value, "%s=%s ", key, temp_value)!=EOF) //keywords
			/*unsigned char key1[256], value1[256], input[256];
			list<string > *tempList;
			tempList = new list<string > ();
			while(sscanf((char *)value, "%s=\"%s\"", key1, value1)!=EOF)
			{
				sprintf((char *)input, "%s=%s", key1, value1);
				printf("Input : %s    %s\n", key1, value1);
				tempList->push_back(string((char *)input));
				//tempList->push_back(string(value1));				
			}
			for (list<string >::iterator it = tempList->begin(); it != tempList->end(); ++it){
				printf("Values are: %s\n", (*it).c_str());
			}
			free(tempList);*/
			//printf("value is before: %s\n", value);
			for(int i=0;value[i]!='\0';i++)
			{
				if(value[i] == '"')
				{
					while(value[++i]!='"')
					{
						if(value[i]==' ')
							value[i] = '^';
					}
				}
			}
			//printf("value is now: %s\n", value);
			unsigned char *saveptr1, *saveptr2, *saveptr3;
			
			while((key = (unsigned char *)strtok_r((char *)value, " ", (char **)&saveptr1))!=NULL)
			{
				//printf("String is %s, len %d\n", key, (int)strlen((char *)key));
				/*unsigned char *temp_temp_value = (unsigned char *)malloc(strlen((char *)key));
				sprintf((char *)temp_temp_value, "%s", (char *)temp_temp_value);*/
				//printf("String is %s, len %d\n", temp_value, (int)strlen((char *)temp_value));
				unsigned char *temp_key = (unsigned char *)strtok_r((char *)key, "=", (char **)&saveptr2);
				//puts((char *)key);
				//printf("String is %s, len %d\n", key, (int)strlen((char *)key));
				//unsigned char temp_str[256];
				//sprintf((char *)temp_str, "%s", (char *)key);*/
				string str_str((const char *)temp_key, strlen((char *)temp_key));
				//cout << str_str;
				metadata.keywords->push_back(str_str);
				//metadata.keywords[str_str] = 1;
				
				temp_key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);
				//if(key[0] == '"')
				if(temp_key[0] == '"')
				{
					unsigned char *temp = (unsigned char *)malloc(sizeof(unsigned char)*(strlen((char *)temp_key)-2));
					strncpy((char *)temp, (char *)temp_key+1, strlen((char *)temp_key)-2);
					temp[strlen((char *)temp_key)-2]='\0';
					unsigned char *temp_key_1 = (unsigned char *)strtok_r((char *)temp, "^", (char **)&saveptr3);
					//printf("Key is : %s\n", temp);
					metadata.keywords->push_back(string((char *)temp_key_1));
					//metadata.keywords[string((char *)key)] = 1;
					//unsigned char *temp_temp_key;
					while((temp_key_1 = (unsigned char *)strtok_r(NULL, "^", (char **)&saveptr3))!=NULL) //keywords
					{
						metadata.keywords->push_back(string((char *)temp_key_1));
						//metadata.keywords[string((char *)key)] = 1;
					}
					free(temp);
				}
				else
				{
					metadata.keywords->push_back(string((char *)temp_key));
					//metadata.keywords[string((char *)key)] = 1;
				}
				/*free(temp_value);
				temp_temp_value = (unsigned char *)strtok(NULL, "?");
				if(temp_temp_value == NULL)
					break;*/
				value = NULL;
			}
			
			//populate BitVector
			memset(metadata.bitVector, 0, sizeof(metadata.bitVector));

			for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
			//for (map<string, int >::iterator it = metadata.keywords.begin(); it != metadata.keywords.end(); ++it){
				createBitVector(metadata.bitVector, (unsigned char *)(*it).c_str());
				/*printf("%s:\n", (*it).c_str());
				for(int i=0;i<128;i++)
					printf("%02x", metadata.bitVector[i]);
				printf("\n\n");*/
			}
			
			//Update the global file Number
			updateGlobalFileNumber();

			//determine nonce
			unsigned char password[SHA_DIGEST_LENGTH] ;
			GetUOID( const_cast<char *> ("password"), password, sizeof(password)) ;
			unsigned char passwordFileName[10];
			sprintf((char *)passwordFileName, "%s%d.pass", "files/", globalFileNumber);
		
			FILE *f1 = fopen((char *)passwordFileName, "w");
			fprintf(f1,"%s",password);
			fclose(f1);
		
			strncpy((char *)metadata.nonce, (char *)toSHA1(password), 20);

			writeMetaData(metadata);
			writeData(metadata);
			populateBitVectorIndexMap(metadata.bitVector, globalFileNumber);
			populateSha1IndexMap(metadata.sha1, globalFileNumber);
			populateFileNameIndexMap(metadata.fileName, globalFileNumber);
			initiateStore( MetaDataToStr(metadata), string((char *)metadata.fileName) ) ;
		}
		else if(strncasecmp((char *)inp, "search ", 6) == 0)
		{
			unsigned char *value;
			unsigned char *key = (unsigned char *)strtok((char *)inp, " ");
			key = (unsigned char *)strtok(NULL, "\n");
			/*value = (unsigned char *)malloc(sizeof(unsigned char)*strlen((char *)key));
			memset(value, '\0',sizeof(value));*/
			value = (unsigned char *)strtok((char *)key, "=");
			if(strcasecmp((char *)value, "filename")==0)
			{
				value = (unsigned char *)strtok(NULL, "=");
//				list<int> tempList = fileNameSearch(value);
//				for(list<int>::iterator it = tempList.begin();it!=tempList.end();it++)
//				{
//					printf("File name is: %d\n", (*it));
//					//now updating the FILEID MAP
//					struct metaData metadata = populateMetaData((*it));
//					fileIDMap[string((char *)metadata.fileID, 20)] = (*it);
//				}
				// Initiate the search method with the type filename
				initiateSearch(0x01, value) ;	
				checkFlag = 1;
			}
			else if(strcasecmp((char *)value, "sha1hash")==0)
			{
				value = (unsigned char *)strtok(NULL, "=");
				unsigned char *str = toHex(value, 20);
				initiateSearch(0x02, str) ;	
				checkFlag = 1;
			}
			else if(strcasecmp((char *)value, "keywords")==0)
			{
				
				value = (unsigned char *)strtok(NULL, "\0");
				if(value[0] == '"')
				{
				
					unsigned char *temp = (unsigned char *)malloc(sizeof(unsigned char)*(strlen((char *)value)-2));
					strncpy((char *)temp, (char *)value+1, strlen((char *)value)-2);
					temp[strlen((char *)value)-2]='\0';
					initiateSearch(0x03, temp) ;
					
					free(temp);
					checkFlag = 1;
				}
				else
				{
					initiateSearch(0x03, value) ;
					checkFlag = 1;
				}
			}
			pthread_mutex_lock(&searchMsgLock) ;
			pthread_cond_wait(&searchMsgCV, &searchMsgLock);
			pthread_mutex_unlock(&searchMsgLock) ;
		}
		/*else if(strstr((char *) inp, "find ")!=NULL)
		{
			unsigned char *value = (unsigned char *)strtok(inp, " ");
			value = (unsigned char *)strtok(NULL, "\n");
			unsigned char *temp_value = toHex(value, 20);
			printf("The value is : %d\n", fileIDMap[string((char*)temp_value, 20)]);
		}*/
		else if(strstr((char *) inp, "delete ")!=NULL)
		{
			checkFlag = 0;
			
			unsigned char *value;
			unsigned char temp_str[256];
			unsigned char message[256];
			unsigned char *saveptr1, *saveptr2;
			int toFloodFlag = 0 ;
			list<int > tempList;			
			memset(temp_str, '\0', sizeof(temp_str));
			memset(message, '\0', sizeof(message));
			value = (unsigned char *)strtok_r(inp, " ", (char **)&saveptr1);
			while((value = (unsigned char *)strtok_r(NULL, " ", (char **)&saveptr1))!=NULL)
			{
				if(strstr((char *)value, "FileName=")!=NULL)
				{
					sprintf((char *)temp_str, "%s\r\n", value);
					strcat((char *)message, (char *)temp_str);
					unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
					key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);
					tempList = fileNameSearch(key);
				}
				else if (strstr((char *)value, "Nonce=")!=NULL)
				{
					value = (unsigned char *)strtok((char*)value, "\n");
					sprintf((char *)temp_str, "%s\r\n", value);
					strcat((char *)message, (char *)temp_str);
					unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
					key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);
					key = toHex(key, 20);
					unsigned char passFile[10];
					unsigned char tempPassword[20];
					memset(tempPassword,0, sizeof(tempPassword));
					FILE *f;
					for(list<int> ::iterator it = tempList.begin();it!=tempList.end();it++)
					{
						struct metaData metadata = populateMetaData((*it));
						//printf("Hi %s & nonce: %s\n", key, metadata.nonce);
						
						/*for(int i=0;i<20;i++)
						{
							printf("%02x",metadata.nonce[i]);
						}
						
						for(int i=0;i<20;i++)
						{
							printf("%02x",key[i]);
						}*/

						if(strncmp((char *)key,(char *)metadata.nonce, 20)==0)
						{
							memset(passFile,0, sizeof(passFile));
							memset(tempPassword,0, sizeof(tempPassword));
							sprintf((char *)passFile, "files/%d.pass", (*it));
							f = fopen((char *)passFile, "r");
							if(f!=NULL)
							{
								fread(tempPassword, 20, 1, f);
								//fscanf(f, "%s", tempPassword);
								unsigned char temp_str1[40];
								memset(temp_str1, 0, 40);
								for(int i=0;i<20;i++)
									sprintf((char *)&temp_str1[i*2], "%02x", tempPassword[i]);
								sprintf((char *)temp_str, "Password=%s\r\n", temp_str1);
								toFloodFlag = 1 ;
								strcat((char *)message, (char *)temp_str);
								fclose(f);
								//printf("hi\n\n%s", message);
							}
							else
							{
								//generate random password
								unsigned char val[10];
								memset(val, '\0', 10);
								printf("\nNo one-time password found.\nOkay to use a random password [yes/no]? ");
								scanf("%s", val);
								val[9]='\0';
								if(val[0]=='y' || val[0]=='Y')
								{
									memset(tempPassword,0, sizeof(tempPassword));
									unsigned char temp_str1[40];
									memset(temp_str1, '\0', 40);
									GetUOID( const_cast<char *> ("password"), tempPassword, sizeof(tempPassword)) ;
									for(int i=0;i<20;i++)
										sprintf((char *)&temp_str1[i*2], "%02x", tempPassword[i]);
									sprintf((char *)temp_str, "Password=%s\r\n", temp_str1);
									toFloodFlag = 1 ;
									strcat((char *)message, (char *)temp_str);
								}
								else
									break;
							}
						}
					}
				}
				else if (strstr((char *)value, "SHA1=")!=NULL)
				{
					sprintf((char *)temp_str, "%s\r\n", value);
					strcat((char *)message, (char *)temp_str);				
				}
				saveptr2 = NULL;
			}
			
			printf("\nMessage to be passed is: \n%s", message);
			string tempMes((char *)message) ;
			if (toFloodFlag)
				initiateDelete((unsigned char *)tempMes.c_str()) ;
			struct parsedDeleteMessage pd = parseDeleteMessage(message);
			deleteFile(pd);
		}
		else if(strstr((char *)inp, "get ")!=NULL)
		{
			if(checkFlag==0)
				continue;
			unsigned char *value = (unsigned char *)strtok((char *)inp, " ");
			value = (unsigned char *)strtok(NULL, " ");
			int indexNumber = atoi((char *)value);
			if(getFileIDMap.find(indexNumber)==getFileIDMap.end())
			{
				printf("No such entry\n");
				continue;
			}
			struct metaData metadata = getFileIDMap[indexNumber];
			value = (unsigned char *)strtok(NULL, "\n");
			if(value == NULL)
				strncpy((char *)extFile, (char *)metadata.fileName, strlen((char *)metadata.fileName));
			else
				strncpy((char *)extFile, (char *)value, strlen((char *)value));
			printf("You have found the entry and now flood to get it!!!\n");
			checkFlag = 1;
			initiateGet(metadata) ;
		}

		memset(inp, '\0', 1024) ;
		//cin.clear() ;
		fflush(stdin);
	}
	
	//printf("KeyBoardThread Halted\n");
	free(inp);
	pthread_exit(0);
	return 0;
}

