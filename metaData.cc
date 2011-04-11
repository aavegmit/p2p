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
			strncpy((char *)metadata.nonce, (char *)str, 20);
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

void updateLRU(unsigned int fileNumber)
{
//	list<int >::iterator result = find(cacheLRU.begin(), cacheLRU.end(), (int)fileNumber);
	cacheLRU.remove(fileNumber);
	cacheLRU.push_back(fileNumber);
}
