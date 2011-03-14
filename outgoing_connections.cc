#include "main.h"
#include "iniParser.h"

#ifndef min
#define min(A,B) (((A)>(B)) ? (B) : (A))
#endif /* ~min */

unsigned char *GetUOID(char *obj_type, unsigned char *uoid_buf, unsigned int uoid_buf_sz) ;
map<unsigned char *, struct Packet> MessageDB ;

void *write_thread(void *args){
	long sockfd = (long)args ;
	struct Message mes ;
	unsigned char header[HEADER_SIZE] ;
	unsigned char *buffer ;
	uint32_t len = 0 ;

	while(!shutDown && !(connectionMap[sockfd].shutDown)){
		if(connectionMap[sockfd].MessageQ.size() <= 0){
			pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
			pthread_cond_wait(&connectionMap[sockfd].mesQCv, &connectionMap[sockfd].mesQLock) ;
			if (connectionMap[sockfd].shutDown)
				break ;
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
		if (connectionMap[sockfd].shutDown)
			break ;

		memset(header, 0, HEADER_SIZE) ;

		// Message of type Hello
		if (mes.type == 0xfa){
			printf("Sending Hello Message\n") ;
			header[0] = 0xfa;
			header[21] = '1' ;
			char host[256] ;
			gethostname(host, 256) ;
			host[255] = '\0' ;
			len = strlen(host) + 2 ;
			memcpy((char *)&header[23], &(len), 4) ;

			buffer = (unsigned char *)malloc(len) ;
			memset(buffer, '\0', len) ;
			memcpy((char *)buffer, &(myInfo->portNo), 2) ;





			sprintf((char *)&buffer[2], "%s",  host);


		}
		else if (mes.type == 0xfc){
			printf("Sending JOIN request\n") ;

			char host[256] ;
			gethostname(host, 256) ;
			host[255] = '\0' ;
			len = strlen(host) + 20 ;
			buffer = (unsigned char *)malloc(len) ;
			memset(buffer, '\0', len) ;
			memcpy((char *)buffer, &(myInfo->location), 4) ;
			memcpy((char *)&buffer[4], &(myInfo->portNo), 2) ;
			sprintf((char *)&buffer[6], "%s",  host);
			len = strlen((const char *)buffer) ;

			header[0] = 0xfc;
			printf("UOID: %s\n", GetUOID( const_cast<char *> ("msg"), buffer, len)) ;

			unsigned char *uoid =  GetUOID( const_cast<char *> ("msg"), buffer, len);
			struct Packet pk ;
			pk.status = 0;
			MessageDB[uoid] = pk ;

			memcpy((char *)&header[1], uoid, 20) ;
			memcpy((char *)&header[21], &(myInfo->ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		else if (mes.type == 0xfb){
			printf("Sending JOIN Response..\n") ;

		}
		else if (mes.type == 0xf8){
			printf("Sending KeepAlive request\n") ;

			len = 0;
			buffer = (unsigned char *)malloc(len+1) ;
			memset(buffer, '\0', len) ;
			len = strlen((const char *)buffer) ;

			header[0] = 0xf8;
			printf("UOID: %s\n", GetUOID( const_cast<char *> ("msg"), buffer, len)) ;

			unsigned char *uoid =  GetUOID( const_cast<char *> ("msg"), buffer, len);
			struct Packet pk ;
			pk.status = 0;
			MessageDB[uoid] = pk ;

			memcpy((char *)&header[1], uoid, 20) ;
			header[21]='1';
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}

		//KeepAlive message sending
		//Resst the keepAliveTimer for this connection
		map<int, struct connectionNode>::iterator found = connectionMap.find(sockfd);
		if(connectionMap.find(sockfd)!=connectionMap.end())
			//	if(connectionMap[sockfd].keepAliveTimer!=0)
				connectionMap[sockfd].keepAliveTimer = myInfo->keepAliveTimeOut/2;


		int return_code = (int)write(sockfd, header, HEADER_SIZE) ;
		if (return_code != HEADER_SIZE){
			fprintf(stderr, "Socket Write Error") ;
		}

		return_code = (int)write(sockfd, buffer, len) ;
		if (return_code != (int)len){
			fprintf(stderr, "Socket Write Error") ;
		}



		free(buffer) ;

	}

	printf("Write Thread exiting...\n") ;

	return 0;
}


int connectTo(unsigned char *hostName, unsigned int portN ) {

	struct sockaddr_in serv_addr ;
	struct hostent *host ;
	int nSocket = 0, status;

	host = gethostbyname(const_cast<char *> ((char *)hostName)) ;
	if (host == NULL){
		fprintf(stderr, "Unknown host\n") ;
		return -1 ;
	}


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

unsigned char *GetUOID(char *obj_type, unsigned char *uoid_buf, unsigned int uoid_buf_sz){
	static unsigned long seq_no=(unsigned long)1;
	unsigned char sha1_buf[SHA_DIGEST_LENGTH], str_buf[104];

	snprintf((char *)str_buf, sizeof(str_buf), "%s_%s_%1ld", myInfo->node_instance_id, obj_type, (long)seq_no++);
	SHA1(str_buf, strlen((const char *)str_buf), sha1_buf);
	memset(uoid_buf, 0, uoid_buf_sz);
	memcpy(uoid_buf, sha1_buf,min(uoid_buf_sz,sizeof(sha1_buf)));
	return uoid_buf;

}



// Method to join the network
// Case when then init_neighbor file is not present
void joinNetwork(){
	printf("In join network method\n") ;
	int resSock = -1 ;
	pthread_t re_thread ;
	void *thread_result ;

	for(list<struct beaconList *>::iterator it = myInfo->myBeaconList->begin(); it != myInfo->myBeaconList->end(); it++){
		printf("Trying to connect to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
		resSock = connectTo((*it)->hostName, (*it)->portNo) ; 
		if (resSock == -1 ){
			// Connection could not be established
		}
		else{
			struct connectionNode cn ;
			struct node n;
			n.portNo = (*it)->portNo ;
			strcpy(n.hostname, (const char *)(*it)->hostName) ;
			nodeConnectionMap[n] = resSock ;

			int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
			if (mres != 0){
				perror("Mutex initialization failed");

			}
			int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
			if (cres != 0){
				perror("CV initialization failed") ;
			}

			cn.shutDown = 0 ;

			connectionMap[resSock] = cn ;
			// Push a Join Req type message in the writing queue
			struct Message m ;
			m.type = 0xfc ;
			pushMessageinQ(resSock, m) ;

			// Create a read thread for this connection
			int res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}

			// Create a write thread
			pthread_t wr_thread ;
			res = pthread_create(&wr_thread, NULL, write_thread , (void *)resSock);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}

			break ;
		}
	}

	if (resSock == -1){
		fprintf(stderr,"No Beacon node up\n") ;
		exit(0) ;
	}


	// Join the read thread here
	// Thread Join code taken from WROX Publications
	int res = pthread_join(re_thread, &thread_result);
	if (res != 0) {
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}

	joinTimeOutFlag = 0;
	printf("Join process exiting..\n") ;

}
