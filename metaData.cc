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

void updateGlobalFileNumber()
{

	FILE *f = fopen(".fileNumber", "r");
	if(f==NULL)
	{
		printf("'.fileNumber'...File Does not exist\n");
		exit(0);
	}
	fscanf(f, "%d", &globalFileNumber);
	fclose(f);

	globalFileNumber++;

	f = fopen(".fileNumber", "w");
	if(f==NULL)
	{
		printf("'.fileNumber'...Error!!\n");
		exit(0);
	}
	fprintf(f, "%d", globalFileNumber);
	fclose(f);
}


void writeMetaData(struct metaData metadata)
{
	unsigned char dir[10] = "files/";

	unsigned char fileName[10];
	sprintf((char *)fileName, "%s%d.meta", dir, globalFileNumber);

	FILE *f = fopen((char *)fileName, "w");

	fprintf(f,"%s\n","[metadata]");
	fprintf(f, "%s=%s\n", "FileName", metadata.fileName);
	fprintf(f, "%s=%ld\n", "FileSize", metadata.fileSize);
	fprintf(f, "%s=", "SHA1");
	for(int i=0;i<20;i++)
		fprintf(f, "%02x", metadata.sha1[i]);

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
		for(int i=0;i<20;i++)
			sprintf(&res[len+i*2], "%02x", metadata.sha1[i]);

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




	void writeData(struct metaData metadata)
	{
		char ch;
		unsigned char fileName[10];
		sprintf((char *)fileName, "%s%d.data", "files/", globalFileNumber);

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
				fprintf(stderr, "Value not a hexstring\n") ;
				exit(0) ;
			}

			w[i] = w[i] << 4 ;

			if(str[i*2+1] >= '0' && str[i*2+1] <= '9'  ) {
				w[i] = w[i] | (str[i*2+1] - 48) ;
			}
			else if(str[i*2+1] >= 'a' && str[i*2+1] <= 'f'  ) {
				w[i] = w[i] | (str[i*2+1] - 87) ;
			}
			else{
				fprintf(stderr, "Value not a hexstring\n") ;
				exit(0) ;
			}

		}

		//			puts((char *)w);
		return w;
	}


	struct metaData populateMetaData(int fileNumber)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char fileName[10];
		memset(fileName, '\0', sizeof(fileName));
		unsigned char buffer[1024];
		memset(buffer, '\0', sizeof(buffer));
		unsigned char *key;

		sprintf((char *)fileName, "%s%d.meta", "files/", fileNumber);

		memset(metadata.fileID, '\0', sizeof(metadata.fileID));
		GetUOID( const_cast<char *> ("FileID"), metadata.fileID, sizeof(metadata.fileID)) ;

		FILE *f = fopen((char *)fileName, "r");
		if(f==NULL)
		{
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
				strncpy((char *)metadata.sha1, (char *)str, 20);
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


	struct metaData populateMetaDataFromString(unsigned char *input1)
	{
		struct metaData metadata;
		memset(&metadata, 0, sizeof(metaData));
		unsigned char *buffer;
		unsigned char *key, *saveptr1, *saveptr2;
		unsigned char input[strlen((char *)input1)];
		strncpy((char *)metadata.fileID, (char *)input1, 20);
		strncpy((char *)input, (char *)input1+20, strlen((char *)input1)-20);

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
			removeFromLRU();
		}
		cacheLRU.push_back(fileNumber);
		return 1;
	}

	void removeFromLRU()
	{
		int toBeRemoved = cacheLRU.front();
		struct metaData metadata = populateMetaData(toBeRemoved);
		currentCacheSize -= metadata.fileSize;
		cacheLRU.pop_front();
		unsigned char removeString[256];
		
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "files/%d.data", (toBeRemoved));
		remove((char *)removeString);
	
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "files/%d.meta", toBeRemoved);
		remove((char *)removeString);
	
		memset(removeString, '\0', 256);
		sprintf((char *)removeString, "files/%d.pass", toBeRemoved);
		remove((char *)removeString);
	}

	void writeLRUToFile()
	{
		FILE *f = fopen("cacheLRU", "w");
		for(list<int > :: iterator it = cacheLRU.begin(); it!=cacheLRU.end();it++)
		{
			fprintf(f, "%d ", (*it));
		}
		fclose(f);
	}

	void readLRUFromFile()
	{
		int temp;
		FILE *f = fopen("cacheLRU", "r");
		if(f!=NULL)
		{
			while(fscanf(f, "%d ", &temp)!=EOF)
			{
				cacheLRU.push_back(temp);
			}
			fclose(f);
		}
	}

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
