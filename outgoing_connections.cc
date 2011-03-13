#include "main.h"
#include "iniParser.h"

void *write_thread(void *args){
	long sockfd = (long)args ;
	struct Message mes ;
	unsigned char header[HEADER_SIZE] ;
	unsigned char *buffer ;
	uint32_t len = 0 ;

	while(!shutDown){
		if(connectionMap[sockfd].MessageQ.size() <= 0){
			pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
			pthread_cond_wait(&connectionMap[sockfd].mesQCv, &connectionMap[sockfd].mesQLock) ;
			mes = connectionMap[sockfd].MessageQ.front() ;
			connectionMap[sockfd].MessageQ.pop_front() ;
			pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;

		}
		else{
			pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
			mes = connectionMap[sockfd].MessageQ.front() ;
			connectionMap[sockfd].MessageQ.pop_front() ;
			pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;
		}

		memset(header, 0, HEADER_SIZE) ;

		// Message of type Hello
		if (mes.type == 0xfa){
			printf("Sending Hello Message\n") ;
			header[0] = 0xfa;
			header[21] = '1' ;
			char host[256] ;
			len = strlen(host) + 2 ;
			memcpy((char *)&header[23], &(len), 4) ;

			gethostname(host, 256) ;
			host[255] = '\0' ;
			buffer = (unsigned char *)malloc(strlen(host) + 2) ;
			memset(buffer, '\0', strlen(host)+2) ;
			memcpy((char *)buffer, &(myInfo->portNo), 2) ;
			memcpy((char *)(buffer+2), host, strlen(host)) ;
			//sprintf((char *)&buffer[2], "%s",  host);
			
		}

		int return_code = (int)write(sockfd, header, HEADER_SIZE) ;
		if (return_code != HEADER_SIZE){
			fprintf(stderr, "Socket Write Error") ;
		}

		return_code = (int)write(sockfd, buffer, len) ;
		if (return_code != len){
			fprintf(stderr, "Socket Write Error") ;
		}



	free(buffer) ;

	}

	return 0;
}


int connectTo(unsigned char *hostName, unsigned int portN ) {

	struct sockaddr_in serv_addr ;
	struct hostent *host ;
	int nSocket = 0, status;

	host = gethostbyname(const_cast<char *> ((char *)hostName)) ;


	// Creating the new server
	nSocket = socket(AF_INET, SOCK_STREAM,0) ;

	// Initialising the structure with 0
	memset(&serv_addr,0, sizeof(struct sockaddr_in)) ;

	// Filling up the structure with specifics
	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
	serv_addr.sin_port = htons(portN) ;

	// Connect to the server
	status = connect(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) ;
	if (status < 0){
		perror("Connect") ;
		return -1 ;
	}
	else {
		return nSocket ;

	}


}
