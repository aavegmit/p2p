#include "main.h"
#include "iniParser.h"
#include "signalHandler.h"
#include "indexSearch.h"

#ifndef min
#define min(A,B) (((A)>(B)) ? (B) : (A))
#endif /* ~min */

unsigned char *GetUOID(char *obj_type, unsigned char *uoid_buf, long unsigned int uoid_buf_sz) ;
map<string, struct Packet> MessageDB ;

//Write thread, picks the message from the queue and then process it and send to the destinations
//if no message, it goes to sleep
void *write_thread(void *args){
	long sockfd = (long)args ;
	struct Message mes ;
	unsigned char header[HEADER_SIZE] ;
	unsigned char *buffer ;
	uint32_t len = 0 ;
	//signal(SIGUSR1, my_handler);
	//printf("My Id is write: %d\n", (int)pthread_self());
	//connectionMap[sockfd].myWriteId = pthread_self();

	while(!shutDown && !(connectionMap[sockfd].shutDown)){
		//if no messages in the queue, it goes to sleep, wait
		if(connectionMap[sockfd].MessageQ.size() <= 0){
			pthread_mutex_lock(&connectionMap[sockfd].mesQLock) ;
			pthread_cond_wait(&connectionMap[sockfd].mesQCv, &connectionMap[sockfd].mesQLock) ;
			if (connectionMap[sockfd].shutDown)
			{
				pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;
				break ;
			}
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
		{
			pthread_mutex_unlock(&connectionMap[sockfd].mesQLock) ;	
			break ;
		}

		memset(header, 0, HEADER_SIZE) ;

		// Message originated from here
		if (mes.status == 0){
			unsigned char uoid[SHA_DIGEST_LENGTH] ;
			GetUOID( const_cast<char *> ("msg"), uoid, sizeof(uoid)) ;
			struct Packet pk;
			pk.status = 0 ;
			pk.msgLifeTime = myInfo->msgLifeTime;
			pthread_mutex_lock(&MessageDBLock) ;
			MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ;
			pthread_mutex_unlock(&MessageDBLock) ;
		} 
		// Copy the uoid from the structure into the header
		else if (mes.status == 1){
			//			memcpy((char *)&header[1], mes.uoid, 20) ;
			//pk.status = 1 ;
			for (int i=0 ; i < SHA_DIGEST_LENGTH ; i++)
				header[1+i] = mes.uoid[i] ;

		
		}
		// Done for status message
		else if (mes.status == 2){
			for (int i=0 ; i < SHA_DIGEST_LENGTH ; i++)
				header[1+i] = mes.uoid[i] ;
		}

		// Message of type Hello
		if (mes.type == 0xfa){
			//printf("Sending Hello Message\n") ;
			header[0] = 0xfa;
			header[21] = 0x01 ;
			unsigned char host[256] ;
			gethostname((char *)host, 256) ;
			host[255] = '\0' ;
			len = strlen((char *)host) + 2 ;
			memcpy((char *)&header[23], &(len), 4) ;

			buffer = (unsigned char *)malloc(len) ;
			memset(buffer, '\0', len) ;
			memcpy((char *)buffer, &(myInfo->portNo), 2) ;
			
//			sprintf((char *)&buffer[2], "%s",  host);
			for (int i = 0 ; i < (int)len - 2 ; ++i)
				buffer[2+i] = host[i] ;
			
			//Incrementing Ready STatus
			pthread_mutex_lock(&connectionMapLock) ;
			connectionMap[sockfd].isReady++;
			pthread_mutex_unlock(&connectionMapLock) ;


		}
		// Sending JOIN Request
		else if (mes.type == 0xfc){
			//			printf("Sending JOIN request\n") ;

			if (mes.status == 1){
				buffer = mes.buffer ;
				len = mes.buffer_len ;
			}
			else{
				unsigned char host[256] ;
				gethostname((char *)host, 256) ;
				host[255] = '\0' ;
				len = strlen((char *)host) + 6 ;
				buffer = (unsigned char *)malloc(len) ;
				memset(buffer, '\0', len) ;
				memcpy((unsigned char *)buffer, &(myInfo->location), 4) ;
				memcpy((unsigned char *)&buffer[4], &(myInfo->portNo), 2) ;
				sprintf((char *)&buffer[6], "%s",  host);
			}


			header[0] = 0xfc;


			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		else if (mes.type == 0xfb){
//			printf("Sending JOIN Response..\n") ;

			//originated from somewhere else, just need to forward the message
			if (mes.status == 1){
				buffer = (unsigned char *)malloc(mes.buffer_len) ;
				for (int i = 0 ; i < mes.buffer_len ; i++)
					buffer[i] = mes.buffer[i] ;
				len = mes.buffer_len ;
			}
			else{
			//originated from here, needs to send to destinations
				unsigned char host[256] ;
				gethostname((char *)host, 256) ;
				host[255] = '\0' ;
				len = strlen((char *)host) + 26 ;
				buffer = (unsigned char *)malloc(len) ;
				memset(buffer, '\0', len) ;
				//memcpy(buffer, mes.uoid, 20) ;
				for(unsigned int i = 0;i<20;i++){
					buffer[i] = mes.uoid[i];
					//printf("%02x-", mes.uoid[i]) ;
				}
				//printf("\n") ;
				memcpy(&buffer[20], &(mes.location), 4) ;
				memcpy(&buffer[24], &(myInfo->portNo), 2) ;
				sprintf((char *)&buffer[26], "%s",  host);
			}


			header[0] = 0xfb;
			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;
		}
		else if (mes.type == 0xf8){
//			printf("Sending KeepAlive request from : %d\n", (int)sockfd) ;

			len = 0;
			buffer = (unsigned char *)malloc(len) ;
			memset(buffer, '\0', len) ;

			header[0] = 0xf8;
			header[21]=0x01;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		// Status Message request
		else if (mes.type == 0xac){
			//originted from somewhere else
			if (mes.status == 1){
				buffer = mes.buffer ;
				len = mes.buffer_len ;
			}
			else{
				len = 1 ;
				buffer = (unsigned char *)malloc(len) ;
				memset(buffer, '\0', len) ;
				buffer[0] = mes.status_type ;
			}


			header[0] = 0xac;


			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		// Search Message request
		else if (mes.type == 0xec){
			printf("Sending Search request\n") ;
			if (mes.status == 1){
				buffer = mes.buffer ;
				len = mes.buffer_len ;
			}
			else{
				len = mes.buffer_len + 1 ;
				buffer = (unsigned char *)malloc(len) ;
				memset(buffer, '\0', len) ;
				buffer[0] = mes.query_type ;
			printf("Sending req: %02x\n", mes.query_type) ;
				for (unsigned int i = 1 ; i < len ; ++i)
					buffer[i] = mes.query[i-1] ;
			}


			header[0] = 0xec;


			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		// CHECK Message request
		else if (mes.type == 0xf6){
			if (mes.status == 1){
				buffer = mes.buffer ;
				len = mes.buffer_len ;
			}
			else{
				len = 0 ;
			}

			header[0] = 0xf6;

			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;

		}
		else if (mes.type == 0xab){
//			printf("Sending Status Response..\n") ;

			if (mes.status == 1){
				buffer = (unsigned char *)malloc(mes.buffer_len) ;
				for (int i = 0 ; i < mes.buffer_len ; i++)
					buffer[i] = mes.buffer[i] ;
				len = mes.buffer_len ;
			}
			else{
				unsigned char host[256] ;
				gethostname((char *)host, 256) ;
				host[255] = '\0' ;
				unsigned int len1 = strlen((char *)host) + 24 ;
				buffer = (unsigned char *)malloc(len1) ;
				memset(buffer, 0, len1) ;
				//memcpy(buffer, mes.uoid, 20) ;
				for(unsigned int i=0;i<20;i++)
					buffer[i] = mes.uoid[i];
				uint16_t templen = len1 - 22 ;
				memcpy(&buffer[20], &(templen), 2) ;
				memcpy(&buffer[22], &(myInfo->portNo), 2) ;
				for (int i = 24 ; i < (int)len1 ; ++i)
					buffer[i] = host[i-24] ;
//				sprintf((char *)&buffer[24], "%s",  host);
				len  =  len1 ;
				
				for(map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end() ; ++it){
					unsigned int len2 = strlen((char *)((*it).first.hostname)) + 2  ;
					len += len2+4 ;
					buffer = (unsigned char *)realloc(buffer, len) ;
					++it ;
					if ( it == nodeConnectionMap.end() ){
						unsigned int temlen2 = 0 ;
						memcpy(&buffer[len-len2-4], &(temlen2), 4);
					}
					else{
						memcpy(&buffer[len-len2-4], &(len2), 4);
					}
					--it ;
					memcpy(&buffer[len-len2], &((*it).first.portNo ), 2) ;
					for (int i = len - len2 + 2 ; i < (int)len ; ++i)
						buffer[i] = host[i -(len-len2+2)] ;
//					sprintf((char *)&buffer[len-len2+2], "%s",  host);
				}

			}


			header[0] = 0xab;
			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;
		}
		else if (mes.type == 0xeb){
			printf("Sending Search Response..\n") ;

			if (mes.status == 1){
				buffer = (unsigned char *)malloc(mes.buffer_len) ;
				for (int i = 0 ; i < mes.buffer_len ; i++)
					buffer[i] = mes.buffer[i] ;
				len = mes.buffer_len ;
			}
			else{
				// Construct the response
				len = 20 ;
				buffer = (unsigned char *)malloc(len) ;
				for(unsigned int i=0;i<20;i++)
					buffer[i] = mes.uoid[i];
				list<int> tempList ;
				switch(mes.query_type){
					case 0x01:
						tempList = fileNameSearch(mes.query) ;
						break;
					case 0x02:
						tempList = sha1Search(mes.query) ;
						break ;
					case 0x03:
						tempList = keywordSearch(mes.query) ;
						break ;
				}
				if (tempList.size() == 0){
					printf("No records found\n") ;
//					free(buffer) ;
					continue;
				}
				struct metaData metadata ;
				string metaStr("")  ;
				for(list<int>::iterator it = tempList.begin(); it != tempList.end(); it++){
					metadata = populateMetaData(*it) ;
					fileIDMap[string((char *)metadata.fileID, 20)] = (*it) ;
					metaStr = MetaDataToStr(metadata) ;
					uint32_t len1 = metaStr.size() ;
					len = len + len1 + 24 ;
					buffer = (unsigned char *)realloc(buffer, len) ;

					++it ;
					if( it == tempList.end()){
						uint32_t zeroLen = 0 ;
						memcpy(&buffer[len - len1 - 24], &zeroLen, 4) ;
					}
					else{
						memcpy(&buffer[len - len1 - 24], &len1, 4) ;
					}
					--it ;
					printf("%d\n", len) ;
//					memcpy(&buffer[len - len1 - 24], &len1, 4) ;
					for(unsigned int h = (len-len1-20) ; h < (len - len1) ; ++h)
						buffer[h] = metadata.fileID[h - (len-len1-20)] ;
					for(int h = (len-len1) ; h < len ; ++h){
						buffer[h] = metaStr[h-len+len1] ;
						printf("%c", buffer[h]) ;
					}
//					memcpy(&buffer[len-24], metaStr.c_str(), len1) ;
				}
				

			}


			header[0] = 0xeb;
			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;
		}
		else if (mes.type == 0xf5){
//			printf("Sending CHECK Response..\n") ;

			if (mes.status == 1){
				buffer = (unsigned char *)malloc(mes.buffer_len) ;
				for (int i = 0 ; i < mes.buffer_len ; i++)
					buffer[i] = mes.buffer[i] ;
				len = mes.buffer_len ;
			}
			else{
				unsigned int len1 =  20 ;

				buffer = (unsigned char *)malloc(len1) ;
				memset(buffer, 0, len1) ;

				for (int i = 0 ; i < SHA_DIGEST_LENGTH ; ++i)
					buffer[i] = mes.uoid[i] ;
//				memcpy(buffer, mes.uoid, 20) ;

				len  =  len1 ;

			}

			header[0] = 0xf5;
			memcpy((char *)&header[21], &(mes.ttl), 1) ;
			header[22] = 0x00 ;
			memcpy((char *)&header[23], &(len), 4) ;
		}
		else if (mes.type == 0xf7)
		{
//			printf("Sending Notify message\n");
			len = 1;
			buffer = (unsigned char *)malloc(len) ;
			memset(buffer, '\0', len) ;
			//memcpy(buffer, &mes.errorCode, sizeof(mes.errorCode));
			buffer[0]=mes.errorCode;

			header[0] = 0xf7;

			header[21] = 0x01 ;
			memcpy((char *)&header[23], &(len), 4) ;
		}

		//KeepAlive message sending
		//Resst the keepAliveTimer for this connection
		if(connectionMap.find(sockfd)!=connectionMap.end())
		{
			pthread_mutex_lock(&connectionMapLock) ;
			connectionMap[sockfd].keepAliveTimer = myInfo->keepAliveTimeOut/3;
			pthread_mutex_unlock(&connectionMapLock) ;
		}

		int return_code = 0 ;
		return_code = (int)write(sockfd, header, HEADER_SIZE) ;
		if (return_code != HEADER_SIZE){
			//fprintf(stderr, "Socket Write Error") ;
		}

		return_code = (int)write(sockfd, buffer, len) ;
		if (return_code != (int)len){
			fprintf(stderr, "Socket Write Error") ;
		}

		//logging the message sent from this node or forwarded from this node
		unsigned char *logEntry = NULL;
		if(!(mes.type == 0xfa && (inJoinNetwork || connectionMap[sockfd].joinFlag == 1)))
		{
			pthread_mutex_lock(&logEntryLock) ;						
			if(mes.status== 0 || mes.status == 2 )
				logEntry = createLogEntry('s', sockfd, header, buffer);
			else
				logEntry = createLogEntry('f', sockfd, header, buffer);
			
			if(logEntry!=NULL)
				writeLogEntry(logEntry);	
			pthread_mutex_unlock(&logEntryLock) ;				
		}	

		free(buffer) ;

	}

//	printf("Write Thread exiting...\n") ;

	pthread_exit(0);
	return 0;
}


int connectTo(unsigned char *hostName, unsigned int portN ) {

	struct sockaddr_in serv_addr ;
	struct hostent *host ;
	int nSocket = 0, status;

	host = gethostbyname(const_cast<char *> ((char *)hostName)) ;
	if (host == NULL){
		//fprintf(stderr, "Unknown host\n") ;
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
		//perror("Connect") ;
		return -1 ;
	}
	else {
		return nSocket ;

	}

}


//get the UOID of the messages
unsigned char *GetUOID(char *obj_type, unsigned char *uoid_buf, long unsigned int uoid_buf_sz){
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
//	printf("In join network method\n") ;
	int resSock = -1 ;
	pthread_t re_thread ;
	void *thread_result ;

	for(list<struct beaconList *>::iterator it = myInfo->myBeaconList->begin(); it != myInfo->myBeaconList->end(); it++){
//		printf("Trying to connect to %s:%d\n", (*it)->hostName, (*it)->portNo) ;
		resSock = connectTo((*it)->hostName, (*it)->portNo) ; 
		if (resSock == -1 ){
			// Connection could not be established
		}
		else{
			struct connectionNode cn ;
			struct node n;
			n.portNo = (*it)->portNo ;
			//strcpy(n.hostname, (const char *)(*it)->hostName) ;
			for(unsigned int i =0 ;i<strlen((const char *)(*it)->hostName);i++)
				n.hostname[i] = (*it)->hostName[i];
			n.hostname[strlen((const char *)(*it)->hostName)] = '\0';
			//			nodeConnectionMap[n] = resSock ;

			int mres = pthread_mutex_init(&cn.mesQLock, NULL) ;
			if (mres != 0){
				//perror("Mutex initialization failed");
				writeLogEntry((unsigned char *)"//Mutex initialization failed\n");

			}
			int cres = pthread_cond_init(&cn.mesQCv, NULL) ;
			if (cres != 0){
				//perror("CV initialization failed") ;
				writeLogEntry((unsigned char *)"//CV initialization failed\n");
			}

			cn.shutDown = 0 ;
			cn.n = n;
			pthread_mutex_lock(&connectionMapLock) ;
			connectionMap[resSock] = cn ;
			pthread_mutex_unlock(&connectionMapLock) ;
			// Push a Join Req type message in the writing queue
			struct Message m ;
			m.type = 0xfc ;
			m.status = 0 ;
			m.ttl = myInfo->ttl ;
			pushMessageinQ(resSock, m) ;

			// Create a read thread for this connection
			int res = pthread_create(&re_thread, NULL, read_thread , (void *)resSock);
			if (res != 0) {
				//perror("Thread creation failed");
				writeLogEntry((unsigned char *)"//In Join Network: Read Thread Creation Failed\n");
				exit(EXIT_FAILURE);
			}
			childThreadList.push_front(re_thread);
			// Create a write thread
			pthread_t wr_thread ;
			res = pthread_create(&wr_thread, NULL, write_thread , (void *)resSock);
			if (res != 0) {
				//perror("Thread creation failed");
				writeLogEntry((unsigned char *)"//In Join Network: Write Thread Creation Failed\n");
				exit(EXIT_FAILURE);
			}
			childThreadList.push_front(wr_thread);
			break ;
		}
	}

	if (resSock == -1){
		//fprintf(stderr,"No Beacon node up\n") ;
		writeLogEntry((unsigned char *)"//NO Beacon Node is up, shuting down\n");
		exit(0) ;
	}
	int res;
	// Call the timer thread
	// Thread creation and join code taken from WROX Publications book
	pthread_t t_thread ;
	res = pthread_create(&t_thread, NULL, timer_thread , (void *)NULL);
	if (res != 0) {
		//perror("Thread creation failed");
		writeLogEntry((unsigned char *)"//In Join Network: Timer Thread Creation Failed\n");		
		exit(EXIT_FAILURE);
	}
	childThreadList.push_front(t_thread);

	// Join the read thread here
	// Thread Join code taken from WROX Publications
	for (list<pthread_t >::iterator it = childThreadList.begin(); it != childThreadList.end(); ++it){
		//printf("Value is : %d\n", (pthread_t)(*it).second.myReadId);
		int res = pthread_join((*it), &thread_result);
		if (res != 0) {
			//perror("Thread join failed");
			writeLogEntry((unsigned char *)"//In Join Network: Thread Joining Failed\n");
			exit(EXIT_FAILURE);
		}
	}


	joinTimeOutFlag = 0;
//	printf("Join process exiting..\n") ;

	// Sort the output and write them in the file
	//FILE *fp = fopen("init_neighbor_list", "w") ;
	FILE *fp = fopen((char *)tempInitFile, "w");
	if (fp==NULL){
		//fprintf(stderr, "Error in file open") ;
		writeLogEntry((unsigned char *)"//In Join Network: Failed to open Init_Neighbor_list file\n");
		exit(EXIT_FAILURE);
	}
	unsigned char tempPort[10] ;
	if (joinResponse.size() < myInfo->initNeighbor){
	//if (joinResponse.size() < 1){
		//fprintf(stderr, "Failed to locate minimum number of nodes\n") ;
		writeLogEntry((unsigned char *)"//Failed to locate Init Neighbor number of nodes\n");
		fclose(f_log);
		exit(0) ;
	}
	unsigned int counter = 0;
	for (set<struct joinResNode>::iterator it = joinResponse.begin(); it != joinResponse.end() ; it++, counter++){
	
		if(counter==myInfo->minNeighbor)
			break;

		//printf("Hostname: %s, Port: %d, location: %d\n", (*it).hostname, (*it).portNo, (*it).location) ;
		fputs((char *)(*it).hostname , fp) ;
		fputs(":", fp) ;
		sprintf((char *)tempPort, "%d", (*it).portNo) ;
		fputs((char *)tempPort, fp) ;
		fputs("\n", fp) ;

	}
	fflush(fp) ;
	fclose(fp) ;
	pthread_mutex_lock(&nodeConnectionMapLock) ;
	nodeConnectionMap.erase(nodeConnectionMap.begin(), nodeConnectionMap.end()) ;
	pthread_mutex_unlock(&nodeConnectionMapLock) ;
	childThreadList.clear();
}


// Method to flood the status requests
void getStatus(){

	unsigned char uoid[SHA_DIGEST_LENGTH] ;
	GetUOID( const_cast<char *> ("msg"), uoid, sizeof(uoid)) ;
	//			memcpy((char *)&header[1], uoid, 20) ;


	struct Packet pk;
	pk.status = 0 ;
	pk.msgLifeTime = myInfo->msgLifeTime;


	pthread_mutex_lock(&MessageDBLock) ;
	MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ;
	pthread_mutex_unlock(&MessageDBLock) ;
	
	//sending the status request message to all of it's neighbor
	pthread_mutex_lock(&nodeConnectionMapLock) ;
	for(map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end() ; ++it){
		struct Message m ;
		m.type = 0xac ;
		m.status = 2 ;
		m.ttl = myInfo->ttl ;
		m.status_type = 0x01 ;
		for (int i=0 ; i < SHA_DIGEST_LENGTH ; i++)
			m.uoid[i] = uoid[i] ;
		pushMessageinQ( (*it).second, m) ;
	}
	pthread_mutex_unlock(&nodeConnectionMapLock) ;

}


// Method to initiate the CHECK message
// The methid is invoked when one of the node's
// neighbor dies
void initiateCheck(){

	unsigned char uoid[SHA_DIGEST_LENGTH] ;
	GetUOID( const_cast<char *> ("msg"), uoid, sizeof(uoid)) ;

	struct Packet pk;
	pk.status = 0 ;
	pk.msgLifeTime = myInfo->msgLifeTime;


	pthread_mutex_lock(&MessageDBLock) ;
	MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ;
	pthread_mutex_unlock(&MessageDBLock) ;

//	myInfo->checkResponseTimeout = myInfo->joinTimeOut ;

	
	//printf("here1\n") ;
//	pthread_mutex_lock(&nodeConnectionMapLock) ;
	//printf("here2\n") ;
	for(map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end() ; ++it){
	//printf("here3\n") ;
		struct Message m ;
		m.type = 0xf6 ;
		m.status = 2 ;
		m.ttl = myInfo->ttl ;
		for (int i=0 ; i < SHA_DIGEST_LENGTH ; i++)
			m.uoid[i] = uoid[i] ;
		pushMessageinQ( (*it).second, m) ;
	}
//	pthread_mutex_unlock(&nodeConnectionMapLock) ;
	checkTimerFlag = 1 ;

}


void initiateSearch(unsigned char type, unsigned char *value){
	printf("here\n") ;
	unsigned char uoid[SHA_DIGEST_LENGTH] ;
	GetUOID( const_cast<char *> ("msg"), uoid, sizeof(uoid)) ;


	struct Packet pk;
	pk.status = 0 ;
	pk.msgLifeTime = myInfo->msgLifeTime;


	pthread_mutex_lock(&MessageDBLock) ;
	MessageDB[string((const char *)uoid, SHA_DIGEST_LENGTH) ] = pk ;
	pthread_mutex_unlock(&MessageDBLock) ;

	//sending the status request message to all of it's neighbor
	pthread_mutex_lock(&nodeConnectionMapLock) ;
	for(map<struct node, int>::iterator it = nodeConnectionMap.begin(); it != nodeConnectionMap.end() ; ++it){
		struct Message m ;
		m.type = 0xec ;
		m.status = 2 ;
		m.ttl = myInfo->ttl ;
		m.query_type = type ;
		m.buffer_len = strlen((char *)value) ;
		m.query = (unsigned char *)malloc(m.buffer_len) ;
		for(int l = 0 ; l < m.buffer_len; ++l)
			m.query[l] = value[l] ;
		for (int i=0 ; i < SHA_DIGEST_LENGTH ; i++)
			m.uoid[i] = uoid[i] ;
		pushMessageinQ( (*it).second, m) ;
	}
	pthread_mutex_unlock(&nodeConnectionMapLock) ;
	

}




//Writing the status response to the file, creating the nam file
void writeToStatusFile(){
//	printf("Writing to the file\n") ;
	pthread_mutex_lock(&statusMsgLock) ;
	FILE *fp = fopen((char *)myInfo->status_file, "a") ;
	if (fp == NULL){
		//fprintf(stderr, "File open failed\n") ;
		writeLogEntry((unsigned char *)"//Failed to open STATUS FILE\n");
		exit(0) ;
	}

	set<struct node> distinctNodes ;
	set<struct node>::iterator it1 ;

	for (set< set<struct node> >::iterator it = statusResponse.begin(); it != statusResponse.end() ; ++it){
		it1 = (*it).begin() ;
		distinctNodes.insert( *it1 ) ;
		++it1 ;
		distinctNodes.insert( *it1 ) ;
	}

	if(distinctNodes.size() == 0)
	{
		fputs("n -t * -s ", fp) ;	
		
		unsigned char portS[20] ;
		sprintf((char *)portS, "%d", myInfo->portNo) ;
		fputs((char *)portS, fp) ;
		//Beacon node is represnted by BLUE NODES
		if (myInfo->isBeacon){
			fputs(" -c blue -i blue\n", fp) ;
		}
		//NON-Beacon ndoe is represented by BLACK nodes
		else{
			fputs(" -c black -i black\n", fp) ;
		}		
	}

	for (set< struct node >::iterator it = distinctNodes.begin(); it != distinctNodes.end() ; ++it){
		fputs("n -t * -s ", fp) ;
		unsigned char portS[20] ;
		sprintf((char *)portS, "%d", (*it).portNo) ;
		fputs((char *)portS, fp) ;
		//Beacon node is represnted by BLUE NODES		
		if (isBeaconNode(*it) ){
			fputs(" -c blue -i blue\n", fp) ;
		}
		else{
		//NON-Beacon node is represnted by BLACK NODES
			fputs(" -c black -i black\n", fp) ;
		}
	}

	for (set< set<struct node> >::iterator it = statusResponse.begin(); it != statusResponse.end() ; ++it){
		fputs("l -t * -s ", fp) ;
		struct node n1;
		struct node n2;
		it1 = (*it).begin() ;
		n1 = *it1 ;
		++it1 ;
		n2 = *it1 ;

		unsigned char portS[20] ;
		sprintf((char *) portS, "%d", n1.portNo) ;
		fputs((char *)portS, fp) ;
		fputs(" -d ", fp) ;

		memset(portS, '\0', 20) ;
		sprintf((char *)portS, "%d", n2.portNo) ;
		fputs((char *)portS, fp) ;
		

		if (isBeaconNode(n1) && isBeaconNode(n2) ){
			fputs(" -c blue\n", fp) ;
		}
		else{
			fputs(" -c black\n", fp) ;
		}
	}



	fflush(fp) ;
	fclose(fp) ;
	statusResponse.erase( statusResponse.begin(), statusResponse.end()) ;
	pthread_mutex_unlock(&statusMsgLock) ;
//	printf("status file written\n") ;
}
