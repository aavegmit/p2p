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

#include <algorithm>
// Indexed from 0
// Read a bit from the array
unsigned char readBit(unsigned char *str, int location){
    int byte_loc = location / 8  ;
    int bit_loc = location % 8 ;

    unsigned char temp = str[byte_loc] ;

    temp = temp << bit_loc ;
    temp = temp >> 7 ;
    return temp ;

}

// Write a bit at the specified memory location
void writeBit(unsigned char *str, int location, unsigned char value){
    int byte_loc = location / 8  ;
    int bit_loc = location % 8 ;

    unsigned char temp = 0x80 ;
    temp = temp >> bit_loc ;

    if(value == 0x01){
        str[byte_loc] = str[byte_loc] | temp ;
    }
    else{
        str[byte_loc] = str[byte_loc] & ~temp ;
    }
}

unsigned char *toSHA1(unsigned char *str)
{
unsigned char *sha1_str = (unsigned char *)malloc(sizeof(unsigned char)*20);
	SHA1(str, strlen((const char *)str), sha1_str);
return sha1_str;
}

unsigned char *toMD5(unsigned char *str)
{
unsigned char *md5_str = (unsigned char *)malloc(sizeof(unsigned char)*20);
	MD5(str, strlen((const char *)str), md5_str);
return md5_str;
}


void createBitVector(unsigned char *bitVector, unsigned char *keyword)
{
unsigned short int sha1_int = 0;
unsigned short int md5_int = 0;

for(int i=0;keyword[i]!='\0';i++)
	keyword[i]=tolower(keyword[i]);
	
unsigned char *sha1_str = toSHA1(keyword);
unsigned char *md5_str = toMD5(keyword);
/*printf("Sha1 obtained: \n");
for(int i=0;i<20;i++)
	printf("%02x", sha1_str[i]);
printf("\nMd5 obtained: \n");	
for(int i=0;i<16;i++)
	printf("%02x", md5_str[i]);	
printf("\n");*/
unsigned char sha1_str_temp[2];
unsigned char md5_str_temp[2];

unsigned char temp = readBit(sha1_str, 151);

sha1_str_temp[1] = temp;
sha1_str_temp[0] = sha1_str[19];

temp = readBit(md5_str, 119);
md5_str_temp[1] = temp;
md5_str_temp[0] = md5_str[15];

memcpy(&sha1_int, sha1_str_temp, 2);
memcpy(&md5_int, md5_str_temp, 2);
/*for(int i=0;i<2;i++)
	printf("sha1: %02x, md5: %02x\n", sha1_str_temp[i], md5_str_temp[i]);*/
//unsigned char *bitVector = (unsigned char *)malloc(sizeof(unsigned char)*128);
//unsigned char bitVector[128];
//memset(bitVector, 0, sizeof(bitVector));
/*writeBit(bitVector, 1023 , 0x01);
bitVector = bitVector << md5_int;
bitVector = bitVector << sha1_int;*/
writeBit(bitVector, 1023-(512 + sha1_int) , 0x01);
writeBit(bitVector, 1023-md5_int  , 0x01);

//return bitVector;
}

//Search the files with bitVector
list<int> keywordSearch(unsigned char *completeStr)
{
unsigned char *str = NULL;
unsigned char bitVector[128];
int existFlag = 0;
int checkFlag = 0;
list<string> *tempList = new list<string >();
list<int > returnList;
memset(bitVector, 0, sizeof(bitVector));
		
	while((str = (unsigned char *)strtok((char *)completeStr, " "))!= NULL)
	{
		for(int i=0;str[i]!='\0';i++)
			str[i]=tolower(str[i]);
		tempList->push_back(string((char *)str));
		
		createBitVector(bitVector, str);
		completeStr = NULL;
	}

/*	for(int i=0;i<128;i++)
	{
		printf("%02x", bitVector[i]);
		
	}
*/
	unsigned char tempString[128];
	memset(tempString, 0, sizeof(tempString));
	for (map<string, list<int> >::iterator it = bitVectorIndexMap.begin(); it != bitVectorIndexMap.end(); ++it){
		//strncpy((char *)tempString, (*it).first.c_str(), 128);
		checkFlag=0;
//		printf("\n\nHere\n\n");
		for(int i=0;i<128;i++)
		{
			tempString[i]=((*it).first).c_str()[i];
//			printf("%02x", tempString[i]);
		}
		//printf("Hereagain\n\n");
		for(int i=0;i<128;i++)
		{
			tempString[i] = tempString[i] & bitVector[i];
			if(tempString[i]!=0x00)
			{
				checkFlag = 1;
			}
			//printf("%02x", tempString[i]);
		}
		//printf("\n\nlen is strlen: %d\n", (int)(strlen((char *)tempString)));
		if(checkFlag)
		{
			//now do the keyword search
			for (list<int >::iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); ++it2){
				struct metaData metadata = populateMetaData(*it2);
				for (list<string >::iterator it1 = tempList->begin(); it1 != tempList->end(); ++it1){
				//search for all keywords in each of the file, need to parse the file now and get list of keywords
					list<string >::iterator result = find(metadata.keywords->begin(), metadata.keywords->end(), (*it1));
					if(result!=metadata.keywords->end())
					{
						existFlag = 1;
					}
					else
					{
						existFlag = 0;
						break;
					}
				}
				if(existFlag)
					returnList.push_back((*it2));
			}
		}
	}
return returnList;
}

list<int> fileNameSearch(unsigned char *fileName)
{
	/*for(int i=0;i<(int)buf_len;i++)
		fileName[i]=tolower(fileName[i]);*/
map<string ,list<int> >::iterator it;
list<int> tempList;

	string searchString((const char *)fileName);
	
	it = fileNameIndexMap.find(searchString);
	if(it!=fileNameIndexMap.end())
	{
		for (list<int >::iterator it1 = (*it).second.begin(); it1 != (*it).second.end(); ++it1){
			tempList.push_back((*it1));
		}
		return tempList;
	}
return tempList;
}

list<int> sha1Search(unsigned char *sha1String)
{
map<string ,list<int> >::iterator it;
list<int > tempList;

	string searchString((const char *)sha1String, 20);
	it = sha1IndexMap.find(searchString);

	if(it!=sha1IndexMap.end())
	{
		for (list<int >::iterator it1 = (*it).second.begin(); it1 != (*it).second.end(); ++it1){
			tempList.push_back((*it1));
		}
		return tempList;
	}
	
return tempList;
}


void populateBitVectorIndexMap(unsigned char *bitVector, unsigned int fileNumber)
{
	string str((char *)bitVector, 128);
	bitVectorIndexMap[str].push_back((int)fileNumber);
	for (map<string, list<int> >::iterator it1 = bitVectorIndexMap.begin(); it1 != bitVectorIndexMap.end(); ++it1){
		for(int i=0;i<128;i++)
		{
			//printf("%02x", ((*it1).first).c_str()[i]);
			bitVector[i]=((*it1).first).c_str()[i];
		}
	}
}

void populateSha1IndexMap(unsigned char *sha1, unsigned int fileNumber)
{
/*for(int i=0;i<20;i++)
	printf("%02x", sha1[i]);*/
	
	string str((char *)sha1, 20);
	sha1IndexMap[str].push_back((int)fileNumber);
}

void populateFileNameIndexMap(unsigned char *fileName, unsigned int fileNumber)
{
	/*for(int i=0;fileName[i]!='\0';i++)
		fileName[i]=tolower(fileName[i]);*/

	fileNameIndexMap[string((char *)fileName)].push_back((int)fileNumber);
}

//Prints the Indexed Maps to corresponding files
void writeIndexToFile()
{
//write to Keyword FILE
/*unsigned char kwrd_index[256];
memset(kwrd_index, '\0', sizeof(kwrd_index));

sprintf((char *)kwrd_index, "%s/%s" ,myInfo->homeDir, "kwrd_index");
FILE *f = fopen((char *)kwrd_index, "wb");*/
FILE *f = fopen("kwrd_index", "wb");
unsigned char bitVector_str[129];
for (map<string, list<int > >::iterator it = bitVectorIndexMap.begin(); it != bitVectorIndexMap.end(); ++it){
	//fwrite(&(*it).first,sizeof((*it).first), 1,f);
	//fwrite(&(*it).second,sizeof((*it).second), 1,f);
	//cout<<(*it).first<<endl;
	//printf("\n\n");
	fprintf(f," %d ",(int)((*it).second).size());
	for(int i=0;i<128;i++)
	{
		bitVector_str[i] = (((*it).first).c_str())[i];
		//printf("%02x", bitVector_str[i]);
		fprintf(f, "%c", bitVector_str[i]);
	}
	//bitVector_str[128]='\0';
	
	
	for(list<int >::iterator it1 = (*it).second.begin(); it1 != (*it).second.end(); ++it1){
		fprintf(f, " %d", *it1);
	}
}

//fwrite(&bitVectorIndexMap,sizeof(bitVectorIndexMap), 1,f);
fclose(f);

//write to NAME FILE
/*unsigned char name_index[256];
memset(name_index, '\0', sizeof(name_index));

sprintf((char *)name_index, "%s/%s" ,myInfo->homeDir, "name_index");
f = fopen((char *)name_index, "wb");*/
f = fopen("name_index", "w");
//fwrite(&fileNameIndexMap,sizeof(fileNameIndexMap), 1,f);
for (map<string, list<int> >::iterator it = fileNameIndexMap.begin(); it != fileNameIndexMap.end(); ++it){
	//fwrite(&(*it).first,sizeof((*it).first), 1,f);
	//fwrite(&(*it).second,sizeof((*it).second), 1,f);
	fprintf(f,"%d %s ",(int)((*it).second).size(), ((*it).first).c_str());
	for(list<int >::iterator it1 = (*it).second.begin(); it1 != (*it).second.end(); ++it1){
		fprintf(f, "%d ", *it1);
	}
}
fclose(f);

//write to SHA1 FILE
/*unsigned char sha1_index[256];
memset(sha1_index, '\0', sizeof(sha1_index));

sprintf((char *)sha1_index, "%s/%s" ,myInfo->homeDir, "sha1_index");
f = fopen((char *)sha1_index, "wb");*/
f = fopen("sha1_index", "wb");
for (map<string, list<int> >::iterator it = sha1IndexMap.begin(); it != sha1IndexMap.end(); ++it){
	//fwrite(&(*it).first,sizeof((*it).first), 1,f);
	//fwrite(&(*it).second,sizeof((*it).second), 1,f);
	fprintf(f,"%d ",(int)((*it).second).size());
	for(int i=0;i<20;i++)
		fprintf(f, "%c",((*it).first).c_str()[i]);
	for(list<int >::iterator it1 = (*it).second.begin(); it1 != (*it).second.end(); ++it1){
		fprintf(f, " %d ", *it1);
	}
}
//fwrite(&sha1IndexMap, sizeof(sha1IndexMap), 1,f);
fclose(f);
}

//Populate Index from file
void readIndexFromFile()
{
//write to Keyword FILE
/*unsigned char kwrd_index[256];
memset(kwrd_index, '\0', sizeof(kwrd_index));

sprintf((char *)kwrd_index, "%s/%s" ,myInfo->homeDir, "kwrd_index");
FILE *f = fopen((char *)kwrd_index, "rb");*/

FILE *f = fopen("kwrd_index", "rb");
//fread(&bitVectorIndexMap,sizeof(bitVectorIndexMap), 1,f);
/*for (map<string, list<int > >::iterator it = bitVectorIndexMap.begin(); it != bitVectorIndexMap.end(); ++it){

	fread(&str,sizeof(str), 1,f);
	fread(&(*it).second,sizeof((*it).second), 1,f);
	bitVectorIndexMap[str] = (*it).second;
}*/
int ret = 1;
int size;
list<int > tempList;
unsigned char str[129];
memset(str, 0, sizeof(str));
while(fscanf(f, " %d ",&size)!=EOF)
{
	for(int i=0;i<128;i++)
		fscanf(f, "%c", &str[i]);
	for(int i=0;i<size;i++)
	{
		fscanf(f," %d", &ret);
		tempList.push_back(ret);
	}
	/*ret = (int)fread(&str,sizeof(str), 1,f);
	ret = (int)fread(&tempList,sizeof(tempList), 1,f);*/
	/*for(int i=0;i<128;i++)
		printf("%02x", str[i]);*/
	//cout<<string((char*)str, 128)<<endl;
	bitVectorIndexMap[string((char*)str, 128)] = tempList;
	tempList.clear();
}

fclose(f);
//write to NAME FILE
/*unsigned char name_index[256];
memset(name_index, '\0', sizeof(name_index));

sprintf((char *)name_index, "%s/%s" ,myInfo->homeDir, "name_index");
f = fopen((char *)name_index, "rb");*/
ret=0;
size=0;
unsigned char fileName_str[256];
memset(fileName_str, 0, sizeof(fileName_str));
tempList.clear();
f = fopen("name_index", "r");
while(fscanf(f, "%d %s ",&size,fileName_str)!=EOF)
{
	for(int i=0;i<size;i++)
	{
		fscanf(f,"%d ", &ret);
		tempList.push_back(ret);
	}
	fileNameIndexMap[string((char*)fileName_str, strlen((char *)fileName_str))] = tempList;
	tempList.clear();
}

fclose(f);
//write to SHA1 FILE
/*unsigned char sha1_index[256];
memset(sha1_index, '\0', sizeof(sha1_index));

sprintf((char *)sha1_index, "%s/%s" ,myInfo->homeDir, "sha1_index");
f = fopen((char *)sha1_index, "rb");*/
ret=0;
size=0;
unsigned char sha1_str[20];
memset(sha1_str, 0, sizeof(sha1_str));
tempList.clear();
f = fopen("sha1_index", "rb");
while(fscanf(f, "%d ",&size)!=EOF)
{
	for(int i=0;i<20;i++)
		fscanf(f, "%c",&sha1_str[i]);
	for(int i=0;i<size;i++)
	{
		fscanf(f," %d ", &ret);
		tempList.push_back(ret);
	}
	sha1IndexMap[string((char*)sha1_str, 20)] = tempList;
	tempList.clear();
//printf("size: %d, sha1: %s\n", size, sha1_str);
}
fclose(f);
}
