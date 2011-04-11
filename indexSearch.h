#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <list>
#include <unistd.h>
#include <ctype.h>
#include <map>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>


extern void createBitVector(unsigned char *, unsigned char *keyword);
extern list<int> keywordSearch(unsigned char *completeStr);
extern list<int> fileNameSearch(unsigned char *fileName);
extern list<int> sha1Search(unsigned char *sha1String);
extern void writeIndexToFile();
extern void readIndexFromFile();
unsigned char *toSHA1(unsigned char *str);
unsigned char *toMD5(unsigned char *str);
