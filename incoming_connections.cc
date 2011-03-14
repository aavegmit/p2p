#include "main.h"
#include "iniParser.h"


using namespace std ;

void *accept_connectionsT(void *){
	struct addrinfo hints, *servinfo, *p;
	int nSocket=0 ; // , portGiven = 0 , portNum = 0;
	char portBuf[10] ;
	memset(portBuf, '\0', 10) ;

	int rv ;
	memset(&hints, 0, sizeof hints) ;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	sprintf(portBuf, "%d", myInfo->portNo) ;
	// Code until connection establishment has been taken from the Beej's guide	
	if ((rv = getaddrinfo(NULL, portBuf, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1) ;
	}
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((nSocket = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		int yes = 1 ;
		if (setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(nSocket, p->ai_addr, p->ai_addrlen) == -1) {
			close(nSocket);
			perror("server: bind");
			continue;
		}
		break;
	}

	// Return if fail to bind
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind socket\n");
		exit(1) ;
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (listen(nSocket, 5) == -1) {
		perror("listen");
		exit(1);
	}
	///////////////////////////////////////////////////////////////////////


	for(;;){
		int cli_len = 0, newsockfd = 0;
		struct sockaddr_in cli_addr ;

		// Wait for another nodes to connect
		printf("Node waiting for a connection..\n") ;
		newsockfd = accept(nSocket, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_len ) ;
		//		int erroac = errno ;


		if (newsockfd < 0){
			//			if (erroac == EINTR){
			//				if (shut_alarm){
			//					
			//					for (it = childList.begin(); it != childList.end(); it++){
			//						kill(*it, SIGINT) ;
			//					}
			//				}
			//				else if (shutDown){
			//					for (it = childList.begin(); it != childList.end(); it++){
			//						kill(*it, SIGINT) ;
			//					}
			//				}
			//			}
			//			break ;
			//
		} 
		else {
			printf("\n I got a connection from (%s , %d)", inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

			// Populate connectionMap for this sockfd
			struct connectionNode cn ;

			int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
			if (mres != 0){
				perror("Mutex initialization failed");

			}
			int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
			if (cres != 0){
				perror("CV initialization failed") ;
			}

			cn.shutDown = 0 ;
			connectionMap[newsockfd] = cn ;


			int res ;
			pthread_t re_thread ;
			res = pthread_create(&re_thread, NULL, read_thread , (void *)newsockfd);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}

			pthread_t wr_thread ;
			res = pthread_create(&wr_thread, NULL, write_thread , (void *)newsockfd);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}
			//		close(newsockfd) ;
		}

	}
	int waitV ;
	while(!(wait(&waitV) == -1) ) ;
	printf("Server terminating normally\n") ;
	close(nSocket) ;


	return 0;
}

void process_received_message(int sockfd,uint8_t type, uint8_t ttl, unsigned char *uoid, unsigned char *buffer){


	// Hello message received
	if (type == 0xfa){
		printf("Hello message received\n") ;
		// Break the Tie now
		struct node n ;
		n.portNo = 0 ;
		memcpy((unsigned int *)&n.portNo, buffer, 2) ;
		strcpy(n.hostname, const_cast<char *> ((char *)buffer+2)) ;
		if (!nodeConnectionMap[n])
		{
			nodeConnectionMap[n] = sockfd ;
			int ret = isBeaconNode(n);
			if(!ret)
			{
				printf("Hello\n");
				struct Message m ;
				m.type = 0xfa ;
				pushMessageinQ(sockfd, m);
			}
		}
		else{
		
			if(nodeConnectionMap[n]==sockfd)
				return;
			// break one connection
			if (myInfo->portNo < n.portNo){
				// dissconect this connection
				closeConnection(sockfd) ;
				printf("Have to break jj one connection\n") ;
			}
			else if(myInfo->portNo > n.portNo){
				closeConnection(nodeConnectionMap[n]) ;
				printf("Have to break one connection with %d\n", n.portNo) ;
			}
			else{
				// Compare the hostname here
				char host[256] ;
				gethostname(host, 256) ;
				host[255] = '\0' ;
				//				if (strcmp() > 0){
				//				}
				//				else if(strcmp() < 0){
				//				}
			}
		}


	}
	// Join Request received
	else if(type == 0xfc){
		printf("Join request received\n") ;

		// Check if the message has already been received or not
		if (MessageDB.find(uoid) != MessageDB.end()){
			printf("Message has already been received.\n") ;
			return ;
		}
		struct Packet pk;
		pk.status = 1 ;

		struct node n ;
		n.portNo = 0 ;
		memcpy((unsigned int *)&n.portNo, &buffer[4], 2) ;
		strcpy(n.hostname, const_cast<char *> ((char *)buffer+6)) ;

		unsigned long int location = 0 ;
		memcpy((unsigned int *)&location, buffer, 4) ;

		pk.receivedFrom = n ;
		MessageDB[uoid] = pk ;

		// Respond the sender with the join responce
		struct Message m ;
		m.type = 0xfb ;
		//		pushMessageinQ(sockfd, 0xfb, (myInfo->location - location) ) ;

		// Push the request message in neighbors queue
		for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
			struct Message m ;
			m.type = 0xfb ;
			//			pushMessageinQ((*it).second, 0xfc) ;
		}

	}
	else if (type == 0xfb){
		printf("JOIN response received\n") ;
		if (MessageDB.find(uoid) == MessageDB.end()){
			printf("Message was never forwarded from this node.\n") ;
			return ;
		}
		if (MessageDB[uoid].status == 0){
			// Message was originally sent from this node
		}
		else if(MessageDB[uoid].status == 1){
			// Message was forwarded from this node, see the receivedFrom member
		}
	}
	else if(type == 0xf8)
		printf("Keep Alive Message Received from : %d\n", sockfd);
//KeepAlive message recieved
//Resst the keepAliveTimer for this connection
map<int, struct connectionNode>::iterator found = connectionMap.find(sockfd);
if(connectionMap.find(sockfd)!=connectionMap.end())
	//	if(connectionMap[sockfd].keepAliveTimer!=0)
		connectionMap[sockfd].keepAliveTimeOut = myInfo->keepAliveTimeOut;


}


void *read_thread(void *args){

	long nSocket = (long)args ;
	unsigned char header[HEADER_SIZE];
	unsigned char *buffer ;

	uint8_t message_type=0;
	uint8_t ttl=0;
	uint32_t data_len=0;
	unsigned char uoid[20] ;

	while(!shutDown && !(connectionMap[nSocket].shutDown)  ){
		memset(header, 0, HEADER_SIZE) ;
		memset(uoid, 0, 20) ;

		//Check for the JoinTimeOutFlag
		if(joinTimeOutFlag)
			pthread_exit(0);

		int return_code=(int)read(nSocket, header, HEADER_SIZE);
		
		//Check for the JoinTimeOutFlag
		if(joinTimeOutFlag)
			pthread_exit(0);
		
		if (return_code != HEADER_SIZE){
			printf("Socket Read error...from header\n") ;
			return 0;
		}
		memcpy(&message_type, header, 1);
		memcpy(uoid,       header+1, 20);
		memcpy(&ttl,       header+21, 1);
		memcpy(&data_len,  header+23, 4);


		buffer = (unsigned char *)malloc(sizeof(unsigned char)*(data_len+1)) ;
		memset(buffer, 0, data_len) ;

		//Check for the JoinTimeOutFlag
		if(joinTimeOutFlag)
			pthread_exit(0);

		return_code=(int)read(nSocket, buffer, data_len);

		//Check for the JoinTimeOutFlag
		if(joinTimeOutFlag)
			pthread_exit(0);
		
		if (return_code != (int)data_len){
			printf("Socket Read error...from data\n") ;
			return 0;
		}
		buffer[data_len] = '\0' ;

		process_received_message(nSocket, message_type, ttl,uoid, buffer) ;

		free(buffer) ;
	}



	printf("Read thread exiting..\n") ;

	return 0;
}

void pushMessageinQ(int sockfd, struct Message mes){

	pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
	(connectionMap[sockfd]).MessageQ.push_back(mes) ;
	pthread_cond_signal(&connectionMap[sockfd].mesQCv) ;
	pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;


}

int isBeaconNode(struct node n)
{
	//printf("1. hostname: %s, portno: %d\n", n.hostname,n.portNo);
	for(list<struct beaconList *>::iterator it = myInfo->myBeaconList->begin(); it != myInfo->myBeaconList->end(); it++){
	//printf("hostname: %s, portno: %d\n", (char *)(*it)->hostName,(*it)->portNo);
		if((strcmp(n.hostname, (char *)(*it)->hostName)==0) && (n.portNo == (*it)->portNo))
			return true;
	}
	return false;
}
