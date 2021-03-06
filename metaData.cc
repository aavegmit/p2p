#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <signal.h>
#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"
#include "keepAliveTimer.h"
#include <openssl/md5.h>
#include "indexSearch.h"
#include <strings.h>
#include <algorithm>


// Function to update the global file number in
// the filesystem
int updateGlobalFileNumber()
{
int globalFileNumber = 0;

	unsigned char fileNumber_file[256];
	sprintf((char *)fileNumber_file, "%s/.fileNumber", myInfo->homeDir);

	FILE *f = fopen((char *)fileNumber_file, "r");
	if(f==NULL)
	{
		printf("'.fileNumber'...File Does not exist\n");
		exit(0);
	}
	fscanf(f, "%d", &globalFileNumber);
	fclose(f);

	globalFileNumber++;

	f = fopen((char *)fileNumber_file, "w");
	if(f==NULL)
	{
		printf("'.fileNumber'...Error!!\n");
		exit(0);
	}
	fprintf(f, "%d", globalFileNumber);
	fclose(f);
	
	return globalFileNumber;
}


// Write metadata in the filesystem
void writeMetaData(struct metaData metadata, int globalFileNumber)
{
	//unsigned char dir[10] = "files/";

	unsigned char fileName[256];
	sprintf((char *)fileName, "%s/%d.meta", filesDir, globalFileNumber);

	FILE *f = fopen((char *)fileName, "w");

//	printf("IN WRTIE TO FILE\n\n");
	fprintf(f,"%s\n","[metadata]");
	fprintf(f, "%s=%s\n", "FileName", metadata.fileName);
	fprintf(f, "%s=%ld\n", "FileSize", metadata.fileSize);
	fprintf(f, "%s=", "SHA1");
	for(int i=0;i<20;i++)
	{
		fprintf(f, "%02x", metadata.sha1[i]);
		//printf("%02x", metadata.sha1[i]);
	}

	fprintf(f, "\n%s=", "Nonce");
	for(int i=0;i<20;i++)
		fprintf(f, "%02x", metadata.nonce[i]);

	fprintf(f, "\n%s=", "Keywords");
	for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
		//for (map<string, int >::iterator it = metadata.keywords.begin(); it != metadata.keywords.end(); ++it){
		fprintf(f, "%s ", (*it).c_str());
	}

	fprintf(f, "\n%s=", "Bit-vector");
	for(int i=0;i<128;i++)
		fprintf(f, "%02x", metadata.bitVector[i]);

	//fprintf(f, "\n%s=%s\n", "Bit-vector", metadata.bitVector);
	fclose(f);
}



	string MetaDataToStr(struct metaData metadata)
	{
		string res ;
		res.append("[metadata]\nFileName=") ;
		res.append((char *)metadata.fileName) ;
		res.append("\nFileSize=") ;
		int len = res.size() ;
		res.resize(res.size()+16) ;


		sprintf(&res[len], "%ld\n",  metadata.fileSize);
		res.resize(strlen(res.c_str())) ;
		res.append("SHA1=") ;
		len = res.size() ;
		res.resize(len+40) ;
//printf("\n\n");		
		for(int i=0;i<20;i++)
		{
			sprintf(&res[len+i*2], "%02x", metadata.sha1[i]);
			//printf("%02x", metadata.sha1[i]);
		}

//printf("\n\n");
		res.append("\nNonce=") ;
		len = res.size() ;
		res.resize(len+40) ;
		for(int i=0;i<20;i++)
			sprintf(&res[len+i*2], "%02x", metadata.nonce[i]);

		res.append("\nKeywords=") ;
		len = res.size() ;
		for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
			res.append((*it)) ;
			res.append(" ") ;
		}
		res.resize(res.size() -1) ;

		res.append("\nBit-vector=") ;
		len = res.size() ;
		res.resize(len+256) ;
		for(int i=0;i<128;i++)
			sprintf(&res[len+i*2], "%02x", metadata.bitVector[i]);
		res.resize(res.size()+1) ;
		res[len+256] = '\0' ;
//		res[len+257] = '\0' ;
		len = len +257 ;

		return res ;
		//		strncpy((char *)str, res.c_str(), len) ;

	}




	// Method to write Data or the file content in the filesystem
	void writeData(struct metaData metadata, int globalFileNumber)
	{
		char ch;
		unsigned char fileName[256];
		sprintf((char *)fileName, "%s/%d.data", filesDir, globalFileNumber);

		FILE *f = fopen((char *)metadata.fileName, "rb");
		FILE *f1 = fopen((char *)fileName, "wb");
		while(fread(&ch,1,1,f)!=0)
			fwrite(&ch, 1,1, f1);
		fclose(f);
		fclose(f1);
	}

	unsigned char* toHex(unsigned char *str, int len)
	{
		int l = len;
		unsigned char *w = (unsigned char*)malloc(sizeof(unsigned char)*len) ;

		memset(w, '\0', len) ;
		for(int i=0 ; i < l ; i++){
			if(str[i*2] >= '0' && str[i*2] <= '9'  ) {
				w[i] = str[i*2] - 48 ;
			}
			else if(str[i*2] >= 'a' && str[i*2] <= 'f'  ) {
				w[i] = str[i*2] - 87 ;
			}
			else{
				/*fprintf(stderr, "Value not a hexstring\n") ;
				exit(0) ;*/
			}

			w[i] = w[i] << 4 ;

			if(str[i*2+1] >= '0' && str[i*2+1] <= '9'  ) {
				w[i] = w[i] | (str[i*2+1] - 48) ;
			}
			else if(str[i*2+1] >= 'a' && str[i*2+1] <= 'f'  ) {
				w[i] = w[i] | (str[i*2+1] - 87) ;
			}
			else{
				/*fprintf(stderr, "Value not a hexstring\n") ;
				exit(0) ;*/
			}

		}

		//			puts((char *)w);
		return w;
	}

	void writeToFileFromData(unsigned char fileName[], int fileNumber)
	{
		unsigned char ch;
		unsigned char fileName_temp[256];
		sprintf((char *)fileName_temp, "%s/%d.data", filesDir, fileNumber);

		FILE *f = fopen((char *)fileName_temp, "rb");
		FILE *f1 = fopen((char *)fileName, "wb");
		while(fread(&ch,1,1,f)!=0)
			fwrite(&ch, 1,1, f1);
		fclose(f);
		fclose(f1);
	}
	

	// Populate the metadata data structure from the filesystem
	struct metaData populateMetaData(int fileNumber)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char fileName[256];
		memset(fileName, '\0', sizeof(fileName));
		unsigned char buffer[1024];
		memset(buffer, '\0', sizeof(buffer));
		unsigned char *key;

		sprintf((char *)fileName, "%s/%d.meta", filesDir, fileNumber);

		memset(metadata.fileID, '\0', sizeof(metadata.fileID));
		GetUOID( const_cast<char *> ("FileID"), metadata.fileID, sizeof(metadata.fileID)) ;

		FILE *f = fopen((char *)fileName, "r");
		if(f==NULL)
		{
//			printf("FileName is : %s\n", fileName);
			printf("File does not exist\n");
			exit(0)	;
		}

		while(fgets((char *)buffer, 1023, f)!=NULL)
		{
			if(strstr((char *)buffer, "FileName=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "\n");
				strncpy((char *)metadata.fileName, (char *)key, strlen((char *)key));
			}
			else if(strstr((char *)buffer, "FileSize=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "\n");
				//strncpy(metadata->fileName, key, strlen(key));		
				metadata.fileSize = atol((char *)key);
			}
			else if(strstr((char *)buffer, "SHA1=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "\n");
				unsigned char *str = toHex(key, 20);
				//printf("In populate metadata:\n\n");
				for(int i=0;i<20;i++)
				{
					metadata.sha1[i] = str[i];
					//printf("%02x", metadata.sha1[i]);
				}
				//printf("\n\n");
				//strncpy((char *)metadata.sha1, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Nonce=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "\n");
				unsigned char *str = toHex(key, 20);
				for(int i=0;i<20;i++)
					metadata.nonce[i] = str[i];
				//strncpy((char *)metadata.nonce, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Keywords=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "\n");
				//strncpy(metadata->fileName, key, strlen(key));		
				unsigned char *key1 = NULL;
				metadata.keywords = new list<string>();
				while((key1 = (unsigned char*)strtok((char *)key, " "))!=NULL)
				{
					metadata.keywords->push_back(string((char *)key1));
					key=NULL;
				}
			}
			else if(strstr((char *)buffer, "Bit-vector=")!=NULL)
			{
				key = (unsigned char *)strtok((char *)buffer, "=");
				key = (unsigned char *)strtok(NULL, "=");

				unsigned char *str = toHex(key, 128);
				for(int i=0;i<128;i++)
					metadata.bitVector[i] = str[i];

				free(str);
			}
		}
		fclose(f);
		/*printf("Some values are: \n\n");
		  printf("%s\n%ld\n", metadata.fileName, metadata.fileSize);
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.sha1[i]);
		  printf("\n");
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.nonce[i]);
		  printf("\n");
		  for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
		  printf("%s ", (*it).c_str());
		  }
		  printf("\n");
		  for(int i=0;i<128;i++)
		  printf("%02x", metadata.bitVector[i]);
		  printf("\n");*/
		return metadata;
	}

	/*void updateLRU(unsigned int fileNumber)
	{
		list<int >::iterator result = find(cacheLRU.begin(), cacheLRU.end(), (int)fileNumber);
		if(result!=cacheLRU.end())
		{
			cacheLRU.remove(fileNumber);
			cacheLRU.push_back(fileNumber);
		}
	}*/


	// populate the metadata data structure from a string,
	// usually used to populate the structure from the string received
	// over the socket
	struct metaData populateMetaDataFromString(unsigned char *input1)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char *buffer;
		unsigned char *key, *saveptr1, *saveptr2;
		unsigned char input[strlen((char *)input1)];
		strncpy((char *)metadata.fileID, (char *)input1, 20);
//		strncpy((char *)input, (char *)input1+20, strlen((char *)input1)-20);
		for(int i = 0 ; i < (int)(strlen((char *)input1)-20) ; ++i)
			input[i] = input1[i+20] ;

		/*memset(metadata.fileID, '\0', sizeof(metadata.fileID));
		  GetUOID( const_cast<char *> ("FileID"), metadata.fileID, sizeof(metadata.fileID)) ;*/

		buffer = (unsigned char *)strtok_r((char *)input, "\n", (char **)&saveptr1);
		while((buffer = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr1))!=NULL)
		{
			//printf("value is: %s\n", buffer);
			if(strstr((char *)buffer, "FileName=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				strncpy((char *)metadata.fileName, (char *)key, strlen((char *)key));
			}
			else if(strstr((char *)buffer, "FileSize=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				metadata.fileSize = atol((char *)key);
			}
			else if(strstr((char *)buffer, "SHA1=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				strncpy((char *)metadata.sha1, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Nonce=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=",(char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				strncpy((char *)metadata.nonce, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Keywords=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				unsigned char *key1 = NULL;
				metadata.keywords = new list<string>();
				while((key1 = (unsigned char*)strtok((char *)key, " "))!=NULL)
				{
					metadata.keywords->push_back(string((char *)key1));
					key=NULL;
				}
			}
			else if(strstr((char *)buffer, "Bit-vector=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);

				unsigned char *str = toHex(key, 128);
				for(int i=0;i<128;i++)
					metadata.bitVector[i] = str[i];

				free(str);
			}
		}
		/*printf("Some values are: \n\n");
		  printf("%s\n%ld\n", metadata.fileName, metadata.fileSize);
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.sha1[i]);
		  printf("\n");
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.nonce[i]);
		  printf("\n");
		  for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
		  printf("%s ", (*it).c_str());
		  }
		  printf("\n");
		  for(int i=0;i<128;i++)
		  printf("%02x", metadata.bitVector[i]);
		  printf("\n");*/
		return metadata;
	}



	// Does the same thing as above, just that the argument in this case
	// is C++ string instead of pointer to memory
	struct metaData populateMetaDataFromCPPString(string input1)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char *buffer;
		unsigned char *key, *saveptr1, *saveptr2;
		unsigned char input[input1.size()];
		strncpy((char *)metadata.fileID, (char *)input1.c_str(), 20);
//		strncpy((char *)input, (char *)input1+20, strlen((char *)input1)-20);
		for(int i = 0 ; i < (int)input1.size()-20 ; ++i)
			input[i] = input1[i+20] ;

		/*memset(metadata.fileID, '\0', sizeof(metadata.fileID));
		  GetUOID( const_cast<char *> ("FileID"), metadata.fileID, sizeof(metadata.fileID)) ;*/

		buffer = (unsigned char *)strtok_r((char *)input, "\n", (char **)&saveptr1);
		while((buffer = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr1))!=NULL)
		{
			//printf("value is: %s\n", buffer);
			if(strstr((char *)buffer, "FileName=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				strncpy((char *)metadata.fileName, (char *)key, strlen((char *)key));
			}
			else if(strstr((char *)buffer, "FileSize=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				metadata.fileSize = atol((char *)key);
			}
			else if(strstr((char *)buffer, "SHA1=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				strncpy((char *)metadata.sha1, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Nonce=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=",(char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				strncpy((char *)metadata.nonce, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Keywords=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				unsigned char *key1 = NULL;
				metadata.keywords = new list<string>();
				while((key1 = (unsigned char*)strtok((char *)key, " "))!=NULL)
				{
					metadata.keywords->push_back(string((char *)key1));
					key=NULL;
				}
			}
			else if(strstr((char *)buffer, "Bit-vector=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);

				unsigned char *str = toHex(key, 128);
				for(int i=0;i<128;i++)
					metadata.bitVector[i] = str[i];

				free(str);
			}
		}
		return metadata;
	}


	struct metaData populateMetaDataFromString_noFileID(unsigned char *input)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char *buffer;
		unsigned char *key, *saveptr1, *saveptr2;

		/*memset(metadata.fileID, '\0', sizeof(metadata.fileID));
		  GetUOID( const_cast<char *> ("FileID"), metadata.fileID, sizeof(metadata.fileID)) ;*/

		buffer = (unsigned char *)strtok_r((char *)input, "\n", (char **)&saveptr1);
		strncpy((char *)input, (char *)buffer, strlen((char *)buffer));
		while((buffer = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr1))!=NULL)
		{
			//printf("value is: %s\n", buffer);
			if(strstr((char *)buffer, "FileName=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				strncpy((char *)metadata.fileName, (char *)key, strlen((char *)key));
			}
			else if(strstr((char *)buffer, "FileSize=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				metadata.fileSize = atol((char *)key);
			}
			else if(strstr((char *)buffer, "SHA1=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				//strncpy((char *)metadata.sha1, (char *)str, 20);

				//printf("\n\n%s\n\n",key);
				for(int i=0;i<20;i++)
				{
					//printf("%02x", str[i]);
					metadata.sha1[i] = str[i];
				}
				//printf("\n\n");

				free(str);
			}
			else if(strstr((char *)buffer, "Nonce=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=",(char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				unsigned char *str = toHex(key, 20);
				strncpy((char *)metadata.nonce, (char *)str, 20);
				free(str);
			}
			else if(strstr((char *)buffer, "Keywords=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "\n", (char **)&saveptr2);
				//strncpy(metadata->fileName, key, strlen(key));		
				unsigned char *key1 = NULL;
				metadata.keywords = new list<string>();
				while((key1 = (unsigned char*)strtok((char *)key, " "))!=NULL)
				{
					metadata.keywords->push_back(string((char *)key1));
					key=NULL;
				}
			}
			else if(strstr((char *)buffer, "Bit-vector=")!=NULL)
			{
				key = (unsigned char *)strtok_r((char *)buffer, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);

				unsigned char *str = toHex(key, 128);
				for(int i=0;i<128;i++)
					metadata.bitVector[i] = str[i];

				free(str);
			}
		}
		/*printf("Some values are: \n\n");
		  printf("%s\n%ld\n", metadata.fileName, metadata.fileSize);
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.sha1[i]);
		  printf("\n");
		  for(int i=0;i<20;i++)
		  printf("%02x", metadata.nonce[i]);
		  printf("\n");
		  for (list<string >::iterator it = metadata.keywords->begin(); it != metadata.keywords->end(); ++it){
		  printf("%s ", (*it).c_str());
		  }
		  printf("\n");
		  for(int i=0;i<128;i++)
		  printf("%02x", metadata.bitVector[i]);
		  printf("\n");*/
		return metadata;
	}
	void updateLRU(int fileNumber)
	{
		list<int >::iterator result = find(cacheLRU.begin(), cacheLRU.end(), (int)fileNumber);
		if(result!=cacheLRU.end())
		{
			cacheLRU.erase(result);
			cacheLRU.push_back(fileNumber);
		}
	}

	int storeInLRU(struct metaData metadata, unsigned int fileNumber)
	{
		if(metadata.fileSize > myInfo->cacheSize)
			return -1;
		while((metadata.fileSize + currentCacheSize) > myInfo->cacheSize)
		{
			//removeFromLRU();
			deleteFromIndex(cacheLRU.front());
			//printf("I am in here \n");
		}
		//printf("Isereted into list: %d\n", fileNumber);
		cacheLRU.push_back(fileNumber);
		currentCacheSize+=metadata.fileSize;
		//printf("currentCacheSize is: %d\n", currentCacheSize);
		return 1;
	}

	void removeFromLRU()
	{
		/*int toBeRemoved = cacheLRU.front();
		printf("removed from list: %d\n", toBeRemoved);
		struct metaData metadata = populateMetaData(toBeRemoved);
		//currentCacheSize -= metadata.fileSize;
		//cacheLRU.pop_front();
		
		deleteFromIndex(toBeRemoved);
		
		cacheLRU.remove(toBeRemoved);
		unsigned char removeString[256];
		
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "%s/%d.data", filesDir, (toBeRemoved));
		remove((char *)removeString);
	
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "%s/%d.meta", filesDir, toBeRemoved);
		remove((char *)removeString);
	
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "%s/%d.pass", filesDir, toBeRemoved);
		remove((char *)removeString);*/
	}

	// Externalising the LRU
	void writeLRUToFile()
	{
		unsigned char cacheLRU_file[256];
		sprintf((char *)cacheLRU_file, "%s/cacheLRU", myInfo->homeDir);

		FILE *f = fopen((char *)cacheLRU_file, "w");
		for(list<int > :: iterator it = cacheLRU.begin(); it!=cacheLRU.end();it++)
		{
			fprintf(f, "%d ", (*it));
		}
		fclose(f);
	}

	// Read the LRU from file, usually done at startup
	void readLRUFromFile()
	{
		int temp;
		unsigned char cacheLRU_file[256];
		sprintf((char *)cacheLRU_file, "%s/cacheLRU", myInfo->homeDir);

		FILE *f = fopen((char *)cacheLRU_file, "r");
		if(f!=NULL)
		{
			while(fscanf(f, "%d ", &temp)!=EOF)
			{
				cacheLRU.push_back(temp);
				struct metaData metadata = populateMetaData(temp);
				currentCacheSize += metadata.fileSize;
			}
			fclose(f);
		}
	}

	// Displays the metadata response in the required format
	int searchResponseDisplay(list<struct metaData> metadataList, int count)
	{
		for(list<struct metaData>::iterator it = metadataList.begin(); it!=metadataList.end(); ++it, count++)
		{
			getFileIDMap[count] = (*it);
			printf("[%d] FileID=", count);
			for(int i=0;i<20;i++)
				printf("%02x", (*it).fileID[i]);
			printf("\n    FileName=%s", (*it).fileName);
			printf("\n    FileSize=%ld", (*it).fileSize);
			printf("\n    SHA1=");
			for(int i=0;i<20;i++)
				printf("%02x", (*it).sha1[i]);
			printf("\n    Nonce=");
			for(int i=0;i<20;i++)
				printf("%02x", (*it).nonce[i]);
			printf("\n    Keywords=");
			for(list<string > ::iterator it1 = (*it).keywords->begin(); it1!=(*it).keywords->end();++it1)
			{
				printf("%s ", (*it1).c_str());
			}
			printf("\n");
		}
		return count;
	}
	
	struct parsedDeleteMessage parseDeleteMessage(unsigned char *message)
	{
		struct parsedDeleteMessage pd;
		memset(&pd, 0, sizeof(pd));
		unsigned char *value = NULL;
		unsigned char *saveptr1, *saveptr2;
		while((value = (unsigned char *)strtok_r((char *)message, "\r\n", (char **)&saveptr1))!=NULL)
		{
			if(strstr((char *)value, "FileName=")!=NULL)
			{
				unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);
				strncpy((char *)pd.fileName, (char *)key, strlen((char *)key));
			}
			else if(strstr((char *)value, "SHA1=")!=NULL)
			{
				unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);			
				strncpy((char *)pd.sha1, (char *)toHex(key, 20), 20);
			}
			else if(strstr((char *)value, "Nonce=")!=NULL)
			{
				unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);			
				strncpy((char *)pd.nonce, (char *)toHex(key, 20), 20);
			}
			else if(strstr((char *)value, "Password=")!=NULL)
			{
				unsigned char *key = (unsigned char *)strtok_r((char *)value, "=", (char **)&saveptr2);
				key = (unsigned char *)strtok_r(NULL, "=", (char **)&saveptr2);			
				strncpy((char *)pd.password, (char *)toHex(key, 20), 20);
			}
			message = NULL;
			saveptr2 = NULL;
		}
		return pd;
	}
	
	void deleteFile(struct parsedDeleteMessage pd)
	{
		unsigned char *nonce = (unsigned char *)malloc(sizeof(unsigned char)*20);
		SHA1(pd.password, 20, nonce);

		if(strncmp((char *)nonce, (char *)pd.nonce, 20)==0)
		{
			
			int fileNumber = getFileNumberFromIndex(pd.fileName, pd.nonce);
			if(fileNumber!=-1){
				deleteFromIndex(fileNumber);
			}
		}
	}
	
	int getFileNumberFromIndex(unsigned char *fileName, unsigned char *nonce)
	{
		int ret = -1;
		list<int > tempList = fileNameIndexMap[string((char *)fileName)];
		
		for(list<int> ::iterator it = tempList.begin();it!=tempList.end();it++)
		{
			struct metaData metadata = populateMetaData((*it));
			if(strncmp((char *)nonce,(char *)metadata.nonce, 20)==0)
			{
				return (*it);
			}
		}
	
		return ret;
	}
