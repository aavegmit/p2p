#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"

using namespace std ;

void *accept_connectionsT(void *){
	struct addrinfo hints, *servinfo, *p;
	//int nSocket=0 ; // , portGiven = 0 , portNum = 0;
	unsigned char portBuf[10] ;
	memset(portBuf, '\0', 10) ;

	int rv = 0;
	list<pthread_t > childThreadList_accept;
	memset(&hints, 0, sizeof hints) ;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	sprintf((char *)portBuf, "%d", myInfo->portNo) ;
	// Code until connection establishment has been taken from the Beej's guide	
	if ((rv = getaddrinfo(NULL, (char *)portBuf, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1) ;
	}
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((nSocket_accept = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		int yes = 1 ;
		if (setsockopt(nSocket_accept, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(nSocket_accept, p->ai_addr, p->ai_addrlen) == -1) {
			close(nSocket_accept);
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
	if (listen(nSocket_accept, 5) == -1) {
		perror("listen");
		exit(1);
	}
	///////////////////////////////////////////////////////////////////////


	for(;;){
		int cli_len = 0, newsockfd = 0;
		struct sockaddr_in cli_addr ;


		// Wait for another nodes to connect
		printf("Node waiting for a connection..\n") ;
		if(shutDown)
		{
			//printf("accept halted\n");
			//shutdown(nSocket_accept, SHUT_RDWR);
			break;
			//close(nSocket_accept);
		}

		newsockfd = accept(nSocket_accept, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_len ) ;
		//		int erroac = errno ;

		if(shutDown)
		{
			//printf("accept halted\n");
			//shutdown(nSocket_accept, SHUT_RDWR);
			//close(nSocket_accept);
			break;
		}
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
			printf("\n I got a connection from (%s , %d)", inet_ntoa(cli_addr.sin_addr),(cli_addr.sin_port));

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
			cn.keepAliveTimer = myInfo->keepAliveTimeOut/2;
			cn.keepAliveTimeOut = myInfo->keepAliveTimeOut;
			cn.isReady = 0;
			pthread_mutex_lock(&connectionMapLock) ;
			connectionMap[newsockfd] = cn ;
			pthread_mutex_unlock(&connectionMapLock) ;

			// Send a HELLO message
			struct Message m;
			m.type = 0xfa ;
			m.status = 0;
			m.fromConnect = 0;
			pushMessageinQ(newsockfd, m) ;


			int res ;
			pthread_t re_thread ;
			res = pthread_create(&re_thread, NULL, read_thread , (void *)newsockfd);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}
			connectionMap[newsockfd].myReadId = re_thread;
			childThreadList_accept.push_front(re_thread);

			pthread_t wr_thread ;
			res = pthread_create(&wr_thread, NULL, write_thread , (void *)newsockfd);
			if (res != 0) {
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}
			//		close(newsockfd) ;
			connectionMap[newsockfd].myWriteId = wr_thread;
			childThreadList_accept.push_front(wr_thread);
		}

	}

	/*for (list<pthread_t >::iterator it = childThreadList_accept.begin(); it != childThreadList_accept.end(); ++it){
	  int res = pthread_kill((*it), SIGUSR1);
	  if(res!=0){
	  perror("pthread_kill failed");
	  exit(EXIT_FAILURE);
	//continue;
	}
	}*/
	void *thread_result ;
	for (list<pthread_t >::iterator it = childThreadList_accept.begin(); it != childThreadList_accept.end(); ++it){
		//printf("Value is : %d\n", (pthread_t)(*it).second.myReadId);
		int res = pthread_join((*it), &thread_result);
		if (res != 0) {
			perror("Thread join failed");
			exit(EXIT_FAILURE);
		}
	}
	printf("Server terminating normally\n") ;
	//close(nSocket_accept) ;

	pthread_exit(0);
	return 0;
}

void process_received_message(int sockfd,uint8_t type, uint8_t ttl, unsigned char *uoid, unsigned char *buffer, unsigned int buf_len){


	// Hello message received
	if (type == 0xfa){
		printf("Hello message received\n") ;
		// Break the Tie now
		struct node n ;
		n.portNo = 0 ;
		pthread_mutex_lock(&connectionMapLock) ;
		connectionMap[sockfd].isReady++;
		pthread_mutex_unlock(&connectionMapLock) ;
		memcpy(&n.portNo, buffer, 2) ;
		//strcpy(n.hostname, const_cast<char *> ((char *)buffer+2)) ;
		for(unsigned int i=0;i < (unsigned int) strlen((char *)buffer)-2;i++)
			n.hostname[i] = buffer[i+2];
		n.hostname[strlen((char *)buffer)-2] = '\0';

		pthread_mutex_lock(&nodeConnectionMapLock) ;
		//if (!nodeConnectionMap[n]){
		if (nodeConnectionMap.find(n)==nodeConnectionMap.end()){
			printf("Adding %d in neighbor list\n", n.portNo) ;
			nodeConnectionMap[n] = sockfd ;
		}
		else{

			/*if(nodeConnectionMap.find(n)!=nodeConnectionMap.end()){
				pthread_mutex_unlock(&nodeConnectionMapLock) ;
				return;
			}*/
			// break one connection
			if (myInfo->portNo < n.portNo){
				// dissconect this connection
				closeConnection(sockfd) ;
				nodeConnectionMap[n] = nodeConnectionMap[n] ;
				printf("Have to break jj one connection\n") ;
			}
			else if(myInfo->portNo > n.portNo){
				//				closeConnection(nodeConnectionMap[n]) ;
				nodeConnectionMap[n] = sockfd ;
				printf("Have to break one connection with %d\n", n.portNo) ;
			}
			else{
				// Compare the hostname here
				unsigned char host[256] ;
				gethostname((char *)host, 256) ;
				host[255] = '\0' ;
				
				if (strcmp((char *)host, (char *)n.hostname) < 0){
				
					closeConnection(sockfd) ;
					nodeConnectionMap[n] = nodeConnectionMap[n] ;
					printf("Have to break jj one connection\n") ;
				}
				else if(strcmp((char *)host, (char *)n.hostname) > 0){
				
					nodeConnectionMap[n] = sockfd;
					printf("Have to break one connection with %d\n", n.portNo) ;					
				}
			}
		}
		pthread_mutex_unlock(&nodeConnectionMapLock) ;


	}
	// Join Request received
	else if(type == 0xfc){
		printf("Join request received\n") ;

		// Check if the message has already been received or not
		pthread_mutex_lock(&MessageDBLock) ;
		if (MessageDB.find(string ((const char *)uoid, SHA_DIGEST_LENGTH)   ) != MessageDB.end()){
			printf("Message has already been received.\n") ;
			pthread_mutex_unlock(&MessageDBLock) ;
			return ;
		}
		pthread_mutex_unlock(&MessageDBLock) ;
		struct Packet pk;
		pk.status = 1 ;

		struct node n ;
		n.portNo = 0 ;
		memcpy(&n.portNo, &buffer[4], 2) ;
		//strcpy(n.hostname, const_cast<char *> ((char *)buffer+6)) ;
		for(unsigned int i = 0;i<strlen(((char *)buffer)-6);i++)
			n.hostname[i] = buffer[i+6];
		n.hostname[strlen(((char *)buffer)-6)] = '\0';

		printf("JOIN REQUEST for %s:%d received\n", n.hostname, n.portNo) ;

		uint32_t location = 0 ;
		memcpy(&location, buffer, 4) ;

		pk.receivedFrom = n ;
		pk.status = 1 ;
		pk.sockfd = sockfd ;
		pk.msgLifeTime = myInfo->msgLifeTime;
		pthread_mutex_lock(&MessageDBLock) ;
		MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ; 
		pthread_mutex_unlock(&MessageDBLock) ;

		// Respond the sender with the join response
		struct Message m ;
		m.type = 0xfb ;
		m.status = 0;
		m.ttl = 1 ;
		//strncpy((char *)m.uoid, (const char *)uoid, SHA_DIGEST_LENGTH) ;
		for(unsigned int i = 0;i<SHA_DIGEST_LENGTH;i++)
			m.uoid[i] = uoid[i];

		printf("Location: %d\n", location) ;
		m.location = myInfo->location > location ?  myInfo->location - location : location - myInfo->location ;
		printf("relative Location: %d\n", m.location) ;
		pushMessageinQ(sockfd, m ) ;

		--ttl ;
		// Push the request message in neighbors queue
		if (ttl >= 1 && myInfo->ttl > 0){
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
				if( !((*it).second == sockfd)){
					printf("Join req send to: %d\n", (*it).first.portNo) ;

					struct Message m ;
					m.type = 0xfc ;
					m.ttl = (unsigned int)(ttl) < (unsigned int)myInfo->ttl ? (ttl) : myInfo->ttl  ;
					m.location = location ;
					m.buffer = (unsigned char *)malloc(buf_len) ;
					m.buffer_len = buf_len ;
					for (int i = 0 ; i < (int)buf_len ; i++)
						m.buffer[i] = buffer[i] ;
					m.status = 1 ;
					memset(m.uoid, 0, SHA_DIGEST_LENGTH) ;
					//					memcpy((unsigned char *)m.uoid, (const unsigned char *)uoid, SHA_DIGEST_LENGTH) ;
					//					strncpy((char *)m.uoid, (const char *)uoid, SHA_DIGEST_LENGTH) ;
					for (int i = 0 ; i < 20 ; i++)
						m.uoid[i] = uoid[i] ;
					pushMessageinQ((*it).second, m) ;
				}
			}
			pthread_mutex_unlock(&nodeConnectionMapLock);
		}

	}
	else if (type == 0xfb){
		printf("JOIN response received\n") ;
		unsigned char original_uoid[SHA_DIGEST_LENGTH] ;
		//		memcpy((unsigned char *)original_uoid, buffer, SHA_DIGEST_LENGTH) ;
		for (int i = 0 ; i < SHA_DIGEST_LENGTH ; i++)
			original_uoid[i] = buffer[i] ;

		struct node n ;
		n.portNo = 0 ;
		memcpy(&n.portNo, &buffer[24], 2) ;
		//strcpy(n.hostname, const_cast<char *> ((char *)buffer+26)) ;
		for(unsigned int i = 0;i < buf_len-26;i++)
			n.hostname[i] = buffer[i+26];
		n.hostname[buf_len-26] = '\0';


		uint32_t location = 0 ;
		memcpy(&location, &buffer[20], 4) ;

		if (MessageDB.find(string((const char *)original_uoid, SHA_DIGEST_LENGTH)) == MessageDB.end()){
			printf("JOIN request was never forwarded from this node.\n") ;
			return ;
		}
		else{
			if (MessageDB[string((const char *)original_uoid, SHA_DIGEST_LENGTH)].status == 0){
				struct joinResNode j;
				j.portNo = n.portNo;
				//strncpy(j.hostname, n.hostname, 256) ;
				for(int i=0;i<256;i++)
					j.hostname[i] = n.hostname[i];
				j.location = location ;
				joinResponse.insert(j)  ;
			}
			else if(MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)   ].status == 1){
				printf("Sending back the response to %d\n" ,MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)].receivedFrom.portNo) ;
				// Message was forwarded from this node, see the receivedFrom member
				struct Message m;
				pthread_mutex_lock(&MessageDBLock) ;
				int return_sock = MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)].sockfd ;
				pthread_mutex_unlock(&MessageDBLock) ;

				m.type = type ;
				m.buffer = (unsigned char *)malloc((buf_len + 1)) ;
				m.buffer[buf_len] = '\0' ;
				m.buffer_len = buf_len ;
				for (int i = 0 ; i < (int)buf_len; i++)
					m.buffer[i] = buffer[i] ;
				for (int i = 0 ; i < 20; i++)
					m.uoid[i] = uoid[i] ;
				m.ttl = 1 ;
				m.status = 1 ;
				pushMessageinQ(return_sock, m) ;
			}
		}
	}
	else if(type == 0xf8){
		//		printf("Keep Alive Message Received from : %d\n", sockfd);
	}
	else if(type == 0xac){
		printf("Status request received\n") ;

		// Check if the message has already been received or not
		pthread_mutex_lock(&MessageDBLock) ;
		if (MessageDB.find(string ((const char *)uoid, SHA_DIGEST_LENGTH)   ) != MessageDB.end()){
			printf("Message has already been received.\n") ;
			pthread_mutex_unlock(&MessageDBLock) ;
			return ;
		}
		pthread_mutex_unlock(&MessageDBLock) ;

		uint8_t status_type = 0 ;
		memcpy(&status_type, buffer, 1) ;

		struct Packet pk;
		pk.status = 1 ;
		pk.sockfd = sockfd ;
		pk.msgLifeTime = myInfo->msgLifeTime;
		pthread_mutex_lock(&MessageDBLock) ;
		MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ; 
		pthread_mutex_unlock(&MessageDBLock) ;

		// Respond the sender with the join response
		struct Message m ;
		m.type = 0xab ;
		m.status = 0;
		m.status_type = status_type ;
		m.ttl = 1 ;
		//strncpy((char *)m.uoid, (const char *)uoid, SHA_DIGEST_LENGTH) ;
		for(unsigned int i = 0;i<SHA_DIGEST_LENGTH;i++)
			m.uoid[i] = uoid[i];
			
		pushMessageinQ(sockfd, m ) ;

		--ttl ;
		// Push the request message in neighbors queue
		if (ttl >= 1 && myInfo->ttl > 0){
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
				if( !((*it).second == sockfd)){
					printf("Status req send to: %d\n", (*it).first.portNo) ;

					struct Message m ;
					m.type = 0xac ;
					m.ttl = (unsigned int)(ttl) < (unsigned int)myInfo->ttl ? (ttl) : myInfo->ttl  ;
					m.buffer = (unsigned char *)malloc(buf_len) ;
					m.buffer_len = buf_len ;
					//					strncpy( (char *)m.buffer , (const char *)buffer , sizeof(buffer));
					for (int i = 0 ; i < (int)buf_len ; i++)
						m.buffer[i] = buffer[i] ;
					m.status = 1 ;
					memset(m.uoid, 0, SHA_DIGEST_LENGTH) ;
					//					memcpy((unsigned char *)m.uoid, (const unsigned char *)uoid, SHA_DIGEST_LENGTH) ;
					//					strncpy((char *)m.uoid, (const char *)uoid, SHA_DIGEST_LENGTH) ;
					for (int i = 0 ; i < 20 ; i++)
						m.uoid[i] = uoid[i] ;
					pushMessageinQ((*it).second, m) ;
				}
			}
			pthread_mutex_unlock(&nodeConnectionMapLock);
		}

	}
	else if (type == 0xab){
		printf("Status response received\n") ;
		unsigned char original_uoid[SHA_DIGEST_LENGTH] ;
		//		memcpy((unsigned char *)original_uoid, buffer, SHA_DIGEST_LENGTH) ;
		for (int i = 0 ; i < SHA_DIGEST_LENGTH ; i++)
			original_uoid[i] = buffer[i] ;

		unsigned short len1 = 0 ;
		memcpy((unsigned short *)&len1, &buffer[20], 2) ;


		struct node n ;
		n.portNo = 0 ;
		memcpy((unsigned int *)&n.portNo, &buffer[22], 2) ;
		for(int hh = 0 ; hh < len1 - 2 ; ++hh)
			n.hostname[hh] = buffer[24 + hh] ;
		//		strncpy(n.hostname, const_cast<char *> ((char *)buffer+24) , len1 - 2   ) ;
		n.hostname[len1] = '\0' ;
		printf("%s\n", n.hostname) ;


		if (MessageDB.find(string((const char *)original_uoid, SHA_DIGEST_LENGTH)) == MessageDB.end()){
			printf("Status request was never forwarded from this node.\n") ;
			return ;
		}
		else{
			if (MessageDB[string((const char *)original_uoid, SHA_DIGEST_LENGTH)].status == 0){
				pthread_mutex_lock(&statusMsgLock) ;
				if (statusTimerFlag){
					int i = len1 + 22 ;
					while ( i < (int)buf_len){
						unsigned int templen = 0 ;
						memcpy((unsigned int *)&templen, &buffer[i], 4) ;
						if (templen == 0){
							templen = buf_len - i ;
						}
						struct node n1 ;
						i += 4 ;
						n1.portNo = 0 ;
						memcpy((unsigned int *)&n1.portNo, &buffer[i], 2) ;
						i += 2 ;
						for (int h = 0 ; h < (int)templen - 2 ; ++h)
							n1.hostname[h] = buffer[i+h] ;
						n1.hostname[templen-2] = '\0' ;
						i = i +templen-2;
						//					strncpy(n.hostname, const_cast<char *> ((char *)buffer+i) , templen - 2   ) ;
						printf("%d  <-----> %d\n", n.portNo, n1.portNo) ;
						//			printf("%s\n", n1.hostname) ;
						set <struct node> tempset ;
						tempset.insert(n) ;
						tempset.insert(n1) ;
						statusResponse.insert(tempset) ;

						//	++i ;
					}
				}
				pthread_mutex_unlock(&statusMsgLock) ;

			}
			else if(MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)   ].status == 1){
				// Message was forwarded from this node, see the receivedFrom member
				struct Message m;
				pthread_mutex_lock(&MessageDBLock) ;
				int return_sock = MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)].sockfd ;
				pthread_mutex_unlock(&MessageDBLock) ;

				m.type = type ;
				m.buffer = (unsigned char *)malloc((buf_len + 1)) ;
				m.buffer[buf_len] = '\0' ;
				m.buffer_len = buf_len ;
				for (int i = 0 ; i < (int)buf_len; i++)
					m.buffer[i] = buffer[i] ;
				for (int i = 0 ; i < SHA_DIGEST_LENGTH ; ++i)
					m.uoid[i] = uoid[i] ;
				m.ttl = 1 ;
				m.status = 1 ;
				pushMessageinQ(return_sock, m) ;
			}
		}
	}
	else if(type == 0xf6){
		printf("CHECK message received\n") ;

		// Check if the message has already been received or not
		pthread_mutex_lock(&MessageDBLock) ;
		if (MessageDB.find(string ((const char *)uoid, SHA_DIGEST_LENGTH)   ) != MessageDB.end()){
			printf("Message has already been received.\n") ;
			pthread_mutex_unlock(&MessageDBLock) ;
			return ;
		}
		pthread_mutex_unlock(&MessageDBLock) ;

		unsigned int status_type = 0 ;
		memcpy((unsigned int *)&status_type, buffer, 1) ;

		struct Packet pk;
		pk.status = 1 ;
		pk.sockfd = sockfd ;
		pk.msgLifeTime = myInfo->msgLifeTime;
		pthread_mutex_lock(&MessageDBLock) ;
		MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ; 
		pthread_mutex_unlock(&MessageDBLock) ;

		// Respond the sender with the check response, only if its a beacon node
		if (myInfo->isBeacon){
			struct Message m ;
			m.type = 0xf5 ;
			m.status = 0;
			m.ttl = 1 ;
			for (int i = 0 ; i < SHA_DIGEST_LENGTH ; ++i)
				m.uoid[i] = uoid[i] ;
			//			strncpy((char *)m.uoid, (const char *)uoid, SHA_DIGEST_LENGTH) ;
			pushMessageinQ(sockfd, m ) ;
		}
		else{

			--ttl ;
			// Push the request message in neighbors queue
			if (ttl >= 1 && myInfo->ttl > 0){
				pthread_mutex_lock(&nodeConnectionMapLock) ;
				for (map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end(); ++it){
					if( !((*it).second == sockfd)){
						printf("Check msg send to: %d\n", (*it).first.portNo) ;

						struct Message m ;
						m.type = 0xf6 ;
						m.ttl = (unsigned int)(ttl) < (unsigned int)myInfo->ttl ? (ttl) : myInfo->ttl  ;
						m.buffer = (unsigned char *)malloc(buf_len) ;
						m.buffer_len = buf_len ;
						for (int i = 0 ; i < (int)buf_len ; i++)
							m.buffer[i] = buffer[i] ;
						m.status = 1 ;
						memset(m.uoid, 0, SHA_DIGEST_LENGTH) ;
						for (int i = 0 ; i < 20 ; i++)
							m.uoid[i] = uoid[i] ;
						pushMessageinQ((*it).second, m) ;
					}
				}
				pthread_mutex_unlock(&nodeConnectionMapLock);
			}
		}

	}
	else if (type == 0xf5){
		printf("Check response received\n") ;
		unsigned char original_uoid[SHA_DIGEST_LENGTH] ;
		for (int i = 0 ; i < SHA_DIGEST_LENGTH ; i++)
			original_uoid[i] = buffer[i] ;


		if (MessageDB.find(string((const char *)original_uoid, SHA_DIGEST_LENGTH)) == MessageDB.end()){
			printf("Status request was never forwarded from this node.\n") ;
			return ;
		}
		else{
			if (MessageDB[string((const char *)original_uoid, SHA_DIGEST_LENGTH)].status == 0){
				if (checkTimerFlag){
					// Dsicard fuether messages, since only one respose
					// is required to prove that the node is still connected
					// to the core network
					checkTimerFlag = 0 ;
					printf("Dont worry!! You are still connected to the core SERVANT network\n") ;
				}

			}
			else if(MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)   ].status == 1){
				// Message was forwarded from this node, see the receivedFrom member
				struct Message m;
				pthread_mutex_lock(&MessageDBLock) ;
				int return_sock = MessageDB[ string((const char *)original_uoid, SHA_DIGEST_LENGTH)].sockfd ;
				pthread_mutex_unlock(&MessageDBLock) ;

				m.type = type ;
				m.buffer = (unsigned char *)malloc((buf_len + 1)) ;
				m.buffer[buf_len] = '\0' ;
				m.buffer_len = buf_len ;
				for (int i = 0 ; i < (int)buf_len; i++)
					m.buffer[i] = buffer[i] ;
				m.ttl = 1 ;
				m.status = 1 ;
				pushMessageinQ(return_sock, m) ;
			}
		}
	}
	else if(type == 0xf7)
	{
		unsigned char ch = buffer[0];
		//memcpy(&ch, &buffer, buf_len);
		printf("Recieved Notify message : %02x\n", ch);
		pthread_mutex_lock(&nodeConnectionMapLock) ;
		eraseValueInMap(sockfd);				
		pthread_mutex_unlock(&nodeConnectionMapLock) ;			
		closeConnection(sockfd);
	}
	//KeepAlive message recieved
	//Resst the keepAliveTimer for this connection
	pthread_mutex_lock(&connectionMapLock) ;
	if(connectionMap.find(sockfd)!=connectionMap.end())
		connectionMap[sockfd].keepAliveTimeOut = myInfo->keepAliveTimeOut;
	pthread_mutex_unlock(&connectionMapLock) ;
	//	if(connectionMap[sockfd].keepAliveTimer!=0)
}


void *read_thread(void *args){

	long nSocket = (long)args ;
	unsigned char header[HEADER_SIZE];
	unsigned char *buffer ;
	//signal(SIGUSR1, my_handler);
	//printf("My Id is read: %d\n", (int)pthread_self());

	uint8_t message_type=0;
	uint8_t ttl=0;
	uint32_t data_len=0;
	unsigned char uoid[20] ;
	map<struct node, int>::iterator it;
	//connectionMap[nSocket].myReadId = pthread_self();
	//while(!shutDown && !(connectionMap[nSocket].shutDown)  ){
	while(!(connectionMap[nSocket].shutDown)  ){
		memset(header, 0, HEADER_SIZE) ;
		memset(uoid, 0, 20) ;

		//Check for the JoinTimeOutFlag
		pthread_mutex_lock(&connectionMapLock) ;
		if(joinTimeOutFlag || connectionMap[nSocket].keepAliveTimeOut == -1 || shutDown)
		{
			pthread_mutex_unlock(&connectionMapLock) ;

			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);
			pthread_mutex_unlock(&nodeConnectionMapLock) ;

			closeConnection(nSocket);
			break;
		}
		pthread_mutex_unlock(&connectionMapLock) ;

		int return_code=(int)read(nSocket, header, HEADER_SIZE);
		//Check for the JoinTimeOutFlag
		pthread_mutex_lock(&connectionMapLock) ;
		if(joinTimeOutFlag || connectionMap[nSocket].keepAliveTimeOut == -1 || shutDown)
		{
			pthread_mutex_unlock(&connectionMapLock) ;
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);
			pthread_mutex_unlock(&nodeConnectionMapLock) ;			
			closeConnection(nSocket);
			break;
		}
		pthread_mutex_unlock(&connectionMapLock) ;

		if (return_code != HEADER_SIZE){
			printf("Socket Read error...from header\n") ;
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);				
			pthread_mutex_unlock(&nodeConnectionMapLock) ;			
			closeConnection(nSocket);
			//pthread_exit(0);
			break;
			//return 0;
		}
		memcpy(&message_type, header, 1);
		//memcpy(uoid,       header+1, 20);
		for(int i=0;i<20;i++)
			uoid[i] = header[i+1];
		memcpy(&ttl,       header+21, 1);
		memcpy(&data_len,  header+23, 4);

		/*if(message_type == 0xfb)
			for(int i=0;i<20;i++)
				printf("%02x ", uoid[i]);*/


		buffer = (unsigned char *)malloc(sizeof(unsigned char)*(data_len+1)) ;
		memset(buffer, 0, data_len) ;

		//Check for the JoinTimeOutFlag
		pthread_mutex_lock(&connectionMapLock) ;
		if(joinTimeOutFlag || connectionMap[nSocket].keepAliveTimeOut == -1 || shutDown)
		{
			pthread_mutex_unlock(&connectionMapLock) ;
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);				
			pthread_mutex_unlock(&nodeConnectionMapLock) ;			
			closeConnection(nSocket);
			break;
		}
		pthread_mutex_unlock(&connectionMapLock) ;
		return_code=(int)read(nSocket, buffer, data_len);
		//printf("Reading Buffer on : %d\n", (int)nSocket);		
		//Check for the JoinTimeOutFlag
		pthread_mutex_lock(&connectionMapLock) ;
		if(joinTimeOutFlag || connectionMap[nSocket].keepAliveTimeOut == -1 || shutDown)
		{
			pthread_mutex_unlock(&connectionMapLock) ;
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);				
			pthread_mutex_unlock(&nodeConnectionMapLock) ;			
			closeConnection(nSocket);
			break;
		}
		pthread_mutex_unlock(&connectionMapLock) ;

		if (return_code != (int)data_len){
			printf("Socket Read error...from data\n") ;
			pthread_mutex_lock(&nodeConnectionMapLock) ;
			//it = nodeConnectionMap.find(nSocket);
			//nodeConnectionMap.erase(it);
			eraseValueInMap(nSocket);				
			pthread_mutex_unlock(&nodeConnectionMapLock) ;			
			closeConnection(nSocket);
			break;
		}
		buffer[data_len] = '\0' ;

		process_received_message(nSocket, message_type, ttl,uoid, buffer, data_len) ;

		// Writing data to log file, but first creating log entry
		unsigned char *logEntry = createLogEntry('r', nSocket, header, buffer);
		/*if(logEntry == NULL)
		  printf("Cannot write to file, entry returned is null\n");
		  else*/
		writeLogEntry(logEntry);

		free(buffer) ;
	}



	printf("Read thread exiting..\n") ;

	pthread_exit(0);
	return 0;
}

void notifyMessageSend(int resSock, uint8_t errorCode)
{
	struct Message mes ; 
	memset(&mes, 0, sizeof(mes));
	mes.type = 0xf7 ;
	mes.status = 0 ;
	mes.errorCode = errorCode ;
	//pushMessageinQ(resSock, m) ;

	pthread_mutex_lock(&connectionMap[resSock].mesQLock) ;
	(connectionMap[resSock]).MessageQ.push_front(mes) ;
	pthread_cond_signal(&connectionMap[resSock].mesQCv) ;
	pthread_mutex_unlock(&connectionMap[resSock].mesQLock) ;
}

void pushMessageinQ(int sockfd, struct Message mes){
	pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
	(connectionMap[sockfd]).MessageQ.push_back(mes) ;
	pthread_cond_signal(&connectionMap[sockfd].mesQCv) ;
	pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;
}

int isBeaconNode(struct node n)
{
	for(list<struct beaconList *>::iterator it = myInfo->myBeaconList->begin(); it != myInfo->myBeaconList->end(); it++){
//		printf("port: %d\n", (*it)->portNo);
		if((strcasecmp((char *)n.hostname, (char *)(*it)->hostName)==0) && (n.portNo == (*it)->portNo))
			return true;
	}
	return false;
}
