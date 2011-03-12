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

#define HEADER_SIZE 27


extern bool shutDown ;
//extern map<int, struct connection


// Function declarations
void *keyboard_thread(void *) ;
void *timer_thread(void *) ;
void *accept_connectionsT(void *);	// Waits for other nodes to connect
void *read_thread(void *) ;
void *write_thread(void *) ;
int connectTo(unsigned char *, unsigned int) ;
