/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mServer.c
 * @purpose		: Main Servers for SDFS
 ******************************************************************************/
#include "mServer.h"


int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		fprintf(stderr, "Input Error. Arguments must be as follows:\n");
		fprintf(stderr, "argv[1] will be mDNS ip address\n");
		fprintf(stderr, "argv[2] will be mDNS port\n");
		fprintf(stderr, "argv[3] will be mServer's port to run on\n");
		return -1;
	}
	
	extractArgs(argv);
	init_mLoggingSystem(mLocalIP, mLocalPort);
	createSignalHandler();
	resolveMaster();
	
	mThreadState = THREAD_NOT_RUNNING;
	dictionary_init(&mLocalDir);
	queue_init(&mKeys);
	mDispatcher();
	
	//test1();
	//test2();
	//test3();
	//test4();
	
	return 0;
}

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: monitorHeartbeats
 * @param	: NULL
 * @return	: NULL when thread is finished processing
 * @purpose	: Thread to initialize heartbeat system and recieve heartbeats
 ******************************************************************************/
void *monitorHeartbeats(void *arg)
{
	arg = NULL;
	initHeartbeatSystem(&mServerML,mLocalIP,mLocalPort,&mLocalDir,&mKeys,&mList_mutex,mDNSInfo->mDNS_IP, mDNSInfo->mDNS_port);
	receiveUdpmsg(&mServerML);
	return NULL;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: extractArgs
 * @param	: argv
 * @return	: void
 * @purpose	: Extract and set accordingly args passed to mServer
 ******************************************************************************/
void extractArgs(char *argv[])
{
	//init mDNSInfo
	mDNSInfo = (mDNSInfo_t *)malloc(sizeof(mDNSInfo_t));
	asprintf(&mDNSInfo->mDNS_IP, "%s", argv[1]);
	mDNSInfo->mDNS_port = atoi(argv[2]);
	
	mLocalIP = getLocalIP();
	mLocalPort = atoi(argv[3]);
	
	masterMachineInfo = (masterMachineInfo_t *)malloc(sizeof(masterMachineInfo_t));
	masterMachineInfo->master_IP = NULL;
	masterMachineInfo->master_port = -1;
}

void createSignalHandler()
{
	struct sigaction customSignal;
	customSignal.sa_handler = mSignalHandler;
	customSignal.sa_flags = 0;
	sigemptyset(&customSignal.sa_mask);
	sigaction(SIGINT, &customSignal, NULL);
}

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: resolveMaster
 * @param	: None
 * @return	: Always 0
 * @purpose	: Resolve Ip_addr and port of network master/leader
 ******************************************************************************/
int resolveMaster()
{
	mDNSInfo->socket = createSocketClient(mDNSInfo->mDNS_IP, mDNSInfo->mDNS_port);
	if(mDNSInfo->socket == -1)
	{
		fprintf(stderr, "createSocketClient error: %d\n", mDNSInfo->socket);
		exit(1);
	}
	
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	sendPayload(mPayload, mDNSInfo->socket, VOTE);
	destroyPayload(mPayload);
	
	//get response
	char *mResponse = receiveResponse(mDNSInfo->socket);
	
	//parse the response
	mPayload_t *masterPayload = deserializePayload(mResponse);
	
	if(strcmp(mLocalIP, masterPayload->IP_address) == 0 && (mLocalPort == masterPayload->port))
	{
		mServerType = MASTER;
		mprintf("I Am Master\n");
	}
	else
	{
		mServerType = SERVANT;
		if(masterMachineInfo->master_IP == NULL) //we msut think about case after failure
		{
			asprintf(&masterMachineInfo->master_IP, "%s", masterPayload->IP_address);
			masterMachineInfo->master_port = masterPayload->port;
			mprintf("Master Machine Resolved --> %s:%d\n", masterMachineInfo->master_IP, masterMachineInfo->master_port);
			mprintf("I Am Servant\n");
		}
	}
	return 0;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: getLocalIP
 * @param	: None
 * @return	: String containing local ip
 * @purpose	: Get local network ip address
 ******************************************************************************/
char *getLocalIP()
{
	char *buffer; //to get the local ip address
	struct ifaddrs * ifAddrStruct = NULL;
	struct ifaddrs * ifa = NULL;
	void *tempAddrPtr = NULL;
	
	getifaddrs(&ifAddrStruct);
	if(ifAddrStruct == NULL) return NULL;
	for(ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr->sa_family == AF_INET)
		{
			//ipv4 address
			tempAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tempAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			if(strcmp(ifa->ifa_name, "en1") == 0 || strcmp(ifa->ifa_name, "eth0") == 0)
			{
				asprintf(&buffer, "%s", addressBuffer);
			}
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
		}
	}
	if(ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
	return buffer;
}


void mDispatcher()
{
	//This function will be called in the following cases
	//Case 1: initial launch
	//Case 2: mServerType changes after leader election
	
	//first we must kill running thread
	if(mThreadState == THREAD_RUNNING)
	{
		pthread_kill(monitorNetwork_thread, SIGKILL);
		pthread_kill(monitor_heartbeat_thread, SIGKILL);
		mDestructor(CALL_FROM_mDISPATCHER);
	}
	
	if(mServerType == MASTER)
	{
		mPayload_t *myself = generatePayload(mLocalIP, mLocalPort);
		pthread_mutex_lock(&mList_mutex);
		addToML(&mServerML, myself);
		pthread_mutex_unlock(&mList_mutex);
		pthread_create(&monitor_heartbeat_thread, NULL, monitorHeartbeats, NULL);
	}
	else if(mServerType == SERVANT)
	{
		joinNetwork();
		pthread_create(&monitor_heartbeat_thread, NULL, monitorHeartbeats, NULL);
	}
	else {}
	
	
	int mSocketListen = createSocketServer(mLocalPort);
	queue_init(&mBlockQueue);
	mThreadState = THREAD_RUNNING;
	mprintf("Now monitoring the network...\n");
	while(1)
	{
		mBlock_t *newBlock = (mBlock_t *)malloc(sizeof(mBlock_t));
		newBlock->socket = SOCKET_NOT_SET;
		queue_enqueue(&mBlockQueue, newBlock);
		
		//Listener portion, blocking call
		int mSocket = socketAccept(mSocketListen);
		if(mSocket == -1)
		{
			printf("Socket Accept Error: %s\n", strerror(mSocket));
			exit(EXIT_FAILURE);
		}
		
		//reach here indicating a valid socket connection has been made. We then create
		//a new thread to handle the newly formed connection
		pthread_t newThread;
		newBlock->tid = newThread;
		newBlock->socket = mSocket;
		pthread_create(&newThread, NULL, monitorNetwork, (void *)newBlock);
	}
	socketClose(mSocketListen);
}

void *monitorNetwork(void *arg)
{
	mBlock_t *mBlock = (mBlock_t *)arg;
	int mSocket = mBlock->socket;
	while(1) //for persistent connection
	{
		char *mSerializedMsg = receiveResponse(mSocket);
		if(mSerializedMsg == NULL)
		{
			return NULL;
		}
		
		//next we parse the request
		int payloadAction = getPayloadAction(mSerializedMsg);
		
		mPayload_t *mPayload;
		if(payloadAction != PUT_DIR_FROM_MASTER && payloadAction != PUT_CHUNK_FROM_MASTER)
		{
			mPayload = deserializePayload(mSerializedMsg);
			free(mSerializedMsg);
		}
		
		if(payloadAction == JOIN)
		{
			mprintf("[%s:%d] has JOINED the network\n", mPayload->IP_address, mPayload->port);
			mPayload->socket = mSocket;
			
                        int sizeofmlp = 0;
			//Next we add to our membership list
			pthread_mutex_lock(&mList_mutex);
			addToML(&mServerML, mPayload);
			sizeofmlp = queue_size(&mServerML);
			pthread_mutex_unlock(&mList_mutex);
			
			//next we muticast the join accordingly
			if(mServerType == MASTER)
			{
				multicastMsg(&mServerML, mPayload, payloadAction, mSocket);
				findPredecessor(&mServerML,mLocalIP,mLocalPort,1);
				if (suc == -1)//this is the second server join
					findSuccessor(&mServerML,mLocalIP,mLocalPort);
			}
			else
			{
				//I'm SERVANT
				if (suc == 0)//in the end of queue
					findSuccessor(&mServerML,mLocalIP,mLocalPort);
			}
			if (sizeofmlp == 2)//make a second replica of all files that master has
				makeSecondReplica();
		}
		else if(payloadAction == LEAVE)
		{
			if(failorleave(LEAVE, mPayload, 0, &mServerML))
				changeleader();
			shutdown(mSocket, SHUT_RDWR);
			return NULL;
		}
		else if(payloadAction == GET_MASTER_DIR)
		{
			if(queue_size(&mKeys) == 0)
			{
				char *zeroResp;
				asprintf(&zeroResp, "ZERO\r\n\r\n");
				send(mSocket, zeroResp, strlen(zeroResp), 0);
				free(zeroResp);
			}
			else
			{
				char *serializedDir = serializeDir(&mLocalDir, &mKeys);
				send(mSocket, serializedDir, strlen(serializedDir), 0);
				free(serializedDir);
			}
		}
		else if(payloadAction == FAILURE)
		{
			if(failorleave(FAILURE, mPayload, 0, &mServerML))
				changeleader();
		}
		else if(payloadAction == PUT_FROM_CLIENT)
		{
			
			time_t start, end;
			start = clock();
			
			//We make sure it does not already exist
			queue_t *temp = (queue_t *)dictionary_get(&mLocalDir, mPayload->IP_address);
			
			if(temp == NULL)
			{
				mprintf("Master Server received file from Client\n");
				mDistributionScheme(mPayload);
			}
			else
			{
				mprintf("Master Server rejecting duplicate keyed file from Client\n");
			}
			destroyPayload(mPayload);
			
			end = clock();
			printf("Task PUT_FROM_CLIENT finished in: %lf\n", (float)(end-start)/CLOCKS_PER_SEC);
		}
		else if(payloadAction == PUT_DIR_FROM_MASTER)
		{
			printf("Servant Server received updated dir listings from Master Server\n");
			
			char *key = extractKeyFromSerializedQ(mSerializedMsg);
			deserializeQ(mSerializedMsg, key, &mLocalDir, &mKeys);
		
			printf("Servant Server has updated local directory listings\n");
			free(mSerializedMsg);
		}
		else if(payloadAction == PUT_CHUNK_FROM_MASTER)
		{
			time_t start, end;
			start = clock();
			
			printf("Servant Server received chunk(s) from Master Server\n");
			//Local Directory Listings already cotains metadata for the chunk
			//All we have to do is write to file and then delete the mChunk
			
			//First we extract the key
			int key_index;
			for(key_index = 0; key_index<(int)strlen(mSerializedMsg); key_index++)
			{
				if(mSerializedMsg[key_index] == '#')
				{
					break;
				}
			}
			char *key = (char *)malloc(key_index + 1);
			strncpy(key, mSerializedMsg, key_index);
			key[key_index] = '\0';
			
			//Next we deserialize the chunk
			mChunk_t *tChunk = deserializeMChunk(mSerializedMsg + key_index + 1);
			
			//Next we write to disk
			char *modifiedFilename;
			asprintf(&modifiedFilename, "[%s:%d]%s_%d", mLocalIP, mLocalPort, key, tChunk->chunkID);
			writeToFile(tChunk->data, tChunk->chunkSize, modifiedFilename);
			free(modifiedFilename);
			
			
			//Next we delete the mChunk and free the socket message
			destroyMChunk(tChunk);
			free(mSerializedMsg);

			end = clock();
			printf("Servant Server has written chunk to disk\nTask PUT_FROM_CLIENT finished in: %lf\n", (float)(end-start)/CLOCKS_PER_SEC);
		}
		else if(payloadAction == GET_FROM_CLIENT)
        {
            //incoming paload will have client IP/port and timestamp will contain fname
            
            //deserializePayload --> get filename (which is the key stored in timestamp)
            //check if key exists in dictionary (much faster than going through mKeys)
            //if NULL, just send back the payloadAction as FILE_NOT_FOUND
            
            //else, we iterate through the queue returned from the dictionary
            //and if type is ORIGINAL, we send request to specified IP/Port with
            //the IP/Port of the client
			
            char *key = mPayload->timestamp;
            queue_t *filesQ = (queue_t *)dictionary_get(&mLocalDir, key);
            if(filesQ == NULL)
            {
                char *tResponse;
                asprintf(&tResponse, "%d\r\n\r\n", FILE_NOT_FOUND);
                send(mSocket, tResponse, strlen(tResponse), 0);
                free(tResponse);
            }
            else
            {
                //First we must send the File Found, so mClient can start recv
                //We get the total_chunks from any mChunk
                int num_chunks = ((mChunk_t *)queue_at(filesQ, 0))->total_chunks;
                char *tResponse;
                asprintf(&tResponse, "%d%d\r\n\r\n", num_chunks, FILE_FOUND);
                send(mSocket, tResponse, strlen(tResponse), 0);
                free(tResponse);
				
				
				
				
                queue_t *tempQ = (queue_t *)malloc(sizeof(queue_t));
                queue_init(tempQ);
				
				
                unsigned int i;
                for(i=0; i<queue_size(filesQ); i++)
                {
                    mChunk_t *tChunk = (mChunk_t *)queue_at(filesQ, i);
                    if(tChunk->chunkType == ORIGINAL)
                    {
                        //first check if ip/port is same as local
                        if(strcmp(tChunk->ip_addr, mLocalIP) == 0 && tChunk->port == mLocalPort)
                        {
                            //meaning this local machine has file
                            sendChunk(tChunk, key, mPayload->IP_address, mPayload->port);
                        }
                        else
                        {
                            //We decide which servant mServers to forward to
				
                            unsigned int j;
                            for(j=0; j<queue_size(tempQ); j++)
                            {
                                mPayload_t *tempP = (mPayload_t *)queue_at(tempQ, j);
                                if(strcmp(tempP->IP_address, tChunk->ip_addr)==0 && tempP->port == tChunk->port)
                                    break;
                            }
                            if(j== queue_size(tempQ))
                            {
                                mPayload_t *tempP = generatePayload(tChunk->ip_addr, tChunk->port);
                                queue_enqueue(tempQ, tempP);
                            }
                        }
                    }
                }
				
                //Next we forward the request
                unsigned int j;
                for(j=0; j<queue_size(tempQ); j++)
                {
                    mPayload_t *tempP = (mPayload_t *)queue_at(tempQ, j);
                    int tSock = createSocketClient(tempP->IP_address, tempP->port);
                    sendPayload(mPayload, tSock, GET_FROM_MASTER);
                    destroyPayload(tempP);
                }
                queue_destroy(tempQ);
            }
        }
		else if(payloadAction == GET_FROM_MASTER)
		{
			//so incoming payload is as is, meaning
			//there is client ip/port, timestamp is filename
			//use key get the queue, (if NULL, FATAL_ERROR)
			//iterate through queue, match on ORGINAL, ip and port
			//should I create a function that returns a queue containing
			//local files?
			
			//need to write a modification of readFromFile to get a
			//mChunk_t *buffer from a single chunk
			
			char *key = mPayload->timestamp;
			queue_t *filesQ = (queue_t *)dictionary_get(&mLocalDir, key);
			if(filesQ == NULL)
			{
				printf("FATAL ERROR: GET_FROM_MASTER recv incorrect key\n");
				return NULL;
			}
			
			queue_t *localFilesQ = getLocalFilesList(filesQ, ORIGINAL);
			unsigned int i;
			for(i=0; i<queue_size(localFilesQ); i++)
			{
				mChunk_t *tChunk = (mChunk_t *)queue_at(localFilesQ, i);
				sendChunk(tChunk, key, mPayload->IP_address, mPayload->port);
			}
			
			
			//The following is not actually freeing the data, just the structure
			queue_destroy(localFilesQ);
			free(localFilesQ);
		}
		else if(payloadAction == FAILURE_SHIFT){
			printf(" I received the shift chunk and filename is %s\n",mPayload->IP_address);
			writeToFile(mPayload->timestamp, strlen(mPayload->timestamp), mPayload->IP_address);
		}
		else if(payloadAction == DELETE_FROM_CLIENT)
		{
			//we must remove from local dir and mKeys, then multicast
			mDeleteFile(&mLocalDir, &mKeys, mPayload->timestamp);
			
			unsigned int i;
			for(i=0; i<queue_size(&mServerML); i++)
			{
				mPayload_t *currPayload = (mPayload_t *)queue_at(&mServerML, i);
				if(strcmp(currPayload->IP_address, mLocalIP) == 0 && currPayload->port == mLocalPort)
				{
					//do nothing
				}
				else
				{
					int sendSock = createSocketClient(currPayload->IP_address, currPayload->port);
					sendPayload(mPayload, sendSock, DELETE_FROM_MASTER);
				}
			}
			destroyPayload(mPayload);
		}
		else if(payloadAction == DELETE_FROM_MASTER)
		{
			//we must remove from local dir and mKeys
			mDeleteFile(&mLocalDir, &mKeys, mPayload->timestamp);
			destroyPayload(mPayload);
		}
		else if(payloadAction == VOTE){}
		else break; //never reached
	}
	return NULL;
}


void mDeleteFile(dictionary_t *incomingD, queue_t *incomingKeys, char *filename)
{
	queue_t *theQ = (queue_t *)dictionary_get(incomingD, filename);
	if(theQ == NULL)
	{
		printf("File not found. Nothing is deleted\n");
		return;
	}
	
	deallocateMFile(theQ);
	dictionary_remove(incomingD, filename);
	unsigned int i;
	for(i=0; i<queue_size(incomingKeys); i++)
	{
		if(strcmp(queue_at(incomingKeys, i), filename) == 0)
		{
			queue_remove_at(incomingKeys, i);
			break;
		}
	}
	printf("Deleted file: %s\n", filename);
}

void makeSecondReplica(){
    unsigned int i;
    unsigned int j;
    queue_t tempKeys;
    queue_init(&tempKeys);
    for(i = 0; i < queue_size(&mKeys); i++){

       char *key = queue_at(&mKeys, i);
       char* tempfilename = malloc(80);
       sprintf(tempfilename, "%s", key);
       tempfilename[strlen(key)] = '\0';
       //printf("tempfile name is %s\n",tempfilename);
       queue_enqueue(&tempKeys, tempfilename);
       //printf("tempfile name is still %s\n",tempfilename);
       queue_remove_at(&mKeys, i);
       i --;
       //free(tempfilename);
       for(j = 0; j < queue_size(&tempKeys); j++){
          //printf("%d at tempKeys is %s and size is %d and current mKeys size is %d and i is %d\n",j,(char *)queue_at(&tempKeys, j),queue_size(&tempKeys),queue_size(&mKeys),i);
       }
    }

    for(i = 0; i < queue_size(&tempKeys); i++){
       char *key = queue_at(&tempKeys, i);
       queue_t *tempQ = (queue_t *)dictionary_get(&mLocalDir, key);
       dictionary_t indexD;// for reorder
       dictionary_init(&indexD);
       int totalnum = 0;
       //printf("going to use %s to extract queue\n",key);
       for (j = 0;j < queue_size(tempQ); j++){
          // printf("queue size is %d\n",queue_size(tempQ));
           mChunk_t *tempC = (mChunk_t *)queue_at(tempQ, j);
           totalnum = tempC->total_chunks;//indempotent
           char *filename;
	   asprintf(&filename, "[%s:%d]%s_%d", tempC->ip_addr, tempC->port, key, tempC->chunkID);
           FILE *mFile = fopen(filename, "rb");
           //printf("going to read from file %s\n",filename);
           readFromFileToMChunk(tempC,mFile);                           
           char *tKey;
	   asprintf(&tKey, "%d", tempC->chunkID);	
	   char *tValue;
	   asprintf(&tValue, "%d", j);
	   dictionary_add(&indexD, tKey, tValue);
       }
       queue_t *mFilesQ = (queue_t *)malloc(sizeof(queue_t));
       queue_init(mFilesQ);
       int k;
       for(k=0; k<totalnum; k++)
       {
           char tKey[16];
	   sprintf(tKey, "%d", k);
	   int theIndex = atoi((char *)dictionary_get(&indexD, tKey));
	   queue_enqueue(mFilesQ, queue_at(tempQ, theIndex));
       }
       char *combined = mJoin(mFilesQ);
       dictionary_remove(&mLocalDir, key);//remove the old queue from dic since we need to reput this file
       

       mPayload_t *mPayload = generatePayload(key, -1);
       free(mPayload->timestamp);
       mPayload->timestamp = combined;     
       mDistributionScheme(mPayload);
       destroyPayload(mPayload);
    } 
    for(j = 0; j < queue_size(&tempKeys); j++){
       char* temp = queue_at(&tempKeys, j);
       free(temp);
    }  
}



queue_t *getLocalFilesList(queue_t *incomingQ, int restrict_flag)
{
	queue_t *localListQ = (queue_t *)malloc(sizeof(queue_t)); //must be freed
	queue_init(localListQ);
	
	unsigned int i;
	for(i=0; i<queue_size(incomingQ); i++)
	{
		mChunk_t *tChunk = (mChunk_t *)queue_at(incomingQ, i);
		if(strcmp(tChunk->ip_addr, mLocalIP) == 0 && tChunk->port == mLocalPort)
		{
			if(restrict_flag == NO_RESTRICT)
			{
				queue_enqueue(localListQ, tChunk);
			}
			else
			{
				if(tChunk->chunkType == restrict_flag)
					queue_enqueue(localListQ, tChunk); //just pointers
			}
		}
	}
	return localListQ;
}



void mDistributionScheme(mPayload_t *inPyld)
{
	queue_t *overallQ = (queue_t *)malloc(sizeof(queue_t));
	queue_init(overallQ); //this is the Queue we add to our dictionary
	
	//recall that due to the Hack, timestamp is actually the buffer
	//and ip_address is sdfsfilename
	//The split factor is equal to the number of mServer instances
	
	
	//==========CRITICAL SECTION BEGIN
	pthread_mutex_lock(&mList_mutex);
	int mSplitFactor = (int)queue_size(&mServerML);
	pthread_mutex_unlock(&mList_mutex);
	//==========CRITICAL SECTION END
	
	
	//we must allocate for key/filename because inPyld will be destroyed
	char *sdfs_filename;
	asprintf(&sdfs_filename, "%s", inPyld->IP_address);
	
	
	queue_t *mChunksQ = mSplit(inPyld->timestamp, mSplitFactor, BUFFER);
	//First we want to send
	//Then we free the buffer and set to NULL, then add to dictionary
	
	unsigned int list_index;
	
	for(list_index = 0; list_index < queue_size(&mServerML); list_index++)
	{
		mPayload_t *tPayload = (mPayload_t *)queue_at(&mServerML, list_index);
		int oMI = -1;
		if(mSplitFactor > 1)
			oMI = mReplicationScheme(mSplitFactor, list_index);
		else
		{
			//we have to handle the case when there is no oMI
			//get onlyChunk, set type to ORIGINAL, assign IP/port, add to overallQ
			mChunk_t *onlyChunk = (mChunk_t *)queue_at(mChunksQ, list_index);
			onlyChunk->chunkType = ORIGINAL;
			asprintf(&onlyChunk->ip_addr, "%s", tPayload->IP_address);
			onlyChunk->port = tPayload->port;
			queue_enqueue(overallQ, onlyChunk);
			break;
		}
		
		//So the mChunks we have to send are curr list_index and oMI
		printf("At list_index: %d\n%s:%d--> Will get mChunks [%d,%d]\n", list_index, tPayload->IP_address, tPayload->port, list_index, oMI);
		
		
		//mark the mChunk at listIndex as ORIGINAL and mChunk at oMI as REPLICA
		//in case of REPLICA, make chunk copy and add to overallQ
		
		mChunk_t *originalChunk = (mChunk_t*)queue_at(mChunksQ, list_index);
		mChunk_t *replicaChunk = (mChunk_t *)queue_at(mChunksQ, oMI);
		//do not modify replicaChunk, because it will be an Original chunk on
		//another iteration... Instead make a copy (below).
		//but we will send original and replica
		
		originalChunk->chunkType = ORIGINAL; //idempotent
		mChunk_t *copiedChunk = newCopy_mChunk(replicaChunk); //data not copied
		copiedChunk->chunkType = REPLICA;
		copiedChunk->data = replicaChunk->data; //pointer
		
		//assign IP and Port
		asprintf(&originalChunk->ip_addr, "%s", tPayload->IP_address);
		originalChunk->port = tPayload->port;
		
		asprintf(&copiedChunk->ip_addr, "%s", tPayload->IP_address);
		copiedChunk->port = tPayload->port;
		
		
		//write to overallQ
		queue_enqueue(overallQ, originalChunk);
		queue_enqueue(overallQ, copiedChunk);
	}
	
	//so next we will actually call multicastMFilesQueue
	//and then the iterate and free/NULL
	//and then add to mLocalDir
	int replication_factor = 2;
	multicastMFilesQueue(overallQ, sdfs_filename, replication_factor);
	
	//we must iterate through overallQ and if(data) free and set to NULL
	unsigned int index1;
	for(index1 = 0; index1 < queue_size(overallQ); index1++)
	{
		mChunk_t *currChunk = (mChunk_t *)queue_at(overallQ, index1);
		if(currChunk->chunkType == ORIGINAL &&  currChunk->data)
		{
			free(currChunk->data);
			currChunk->data = NULL;
		}
		if(currChunk->chunkType == REPLICA)
		{
			//this is not really necessary, but just for sanity purposes
			currChunk->data = NULL;
			
			//remember, we don't free if REPLICA because REPLICA's data
			//is simply a pointer to the original
		}
	}
	
	dictionary_add(&mLocalDir, sdfs_filename, overallQ); //dict lib is thread safe
	queue_enqueue(&mKeys, sdfs_filename);
	
	//deallocateMFile(mChunksQ); NO, do not free cuz overallQ will have pointers
	//but you should destroy the queue data strucutre itself.
	queue_destroy(mChunksQ);
}

int mReplicationScheme(int split_factor, int list_index)
{
	//given the current list index and split factor, mReplication scheme
	//will return an int (a one int because replication factor in our
	//replication scheme is 2) corresponding to the other mChunk ID that
	//it needs to send to the current payload assoc. with list_index.
	
	//int other_mChunk_id = ((list_index + 1) + (split_factor - 1))%split_factor;
	//Above equation can be simplified to:
	int other_mChunk_id = (list_index + split_factor)%split_factor;
	if(other_mChunk_id == 0)
		other_mChunk_id = split_factor;
	
	return (other_mChunk_id - 1); //-1 because indexes start at 0
}


void multicastMFilesQueue(queue_t *overallQ, char *key, int replication_factor)
{
	//this function is partially set up to be dynamical; but statically aligned to 2
	
	if(queue_size(overallQ) == 1)
	{
		//straight write to file
		mChunk_t *tempC1 = (mChunk_t *)queue_at(overallQ, 0);
		
		mprintf("Writing chunks to local machine...\n");
		//meaning these are for local machine
		char *filename;
		asprintf(&filename, "[%s:%d]%s_%d", tempC1->ip_addr, tempC1->port, key, tempC1->chunkID);
		writeToFile(tempC1->data, tempC1->chunkSize, filename);
		free(filename);
		printf("Finished writing file to disk.\n");
		return;
	}
	
	
	unsigned int i;
	for(i=0; i<queue_size(overallQ); i+= replication_factor)
	{
		mChunk_t *tempC1 = (mChunk_t *)queue_at(overallQ, i);
		mChunk_t *tempC2 = (mChunk_t *)queue_at(overallQ, i+1);
		if(strcmp(tempC1->ip_addr, tempC2->ip_addr) == 0 && tempC1->port == tempC2->port)
		{
			
			if(strcmp(tempC1->ip_addr, mLocalIP) == 0 && tempC1->port == mLocalPort)
			{
				mprintf("Writing chunks to local machine...\n");
				//meaning these are for local machine
				char *filename;
				asprintf(&filename, "[%s:%d]%s_%d", tempC1->ip_addr, tempC1->port, key, tempC1->chunkID);
				writeToFile(tempC1->data, tempC1->chunkSize, filename);
				free(filename);
				
				char *filename2;
				asprintf(&filename2, "[%s:%d]%s_%d", tempC2->ip_addr, tempC2->port, key, tempC2->chunkID);
				writeToFile(tempC2->data, tempC2->chunkSize, filename2);
				free(filename2);
				printf("Finished writing file to disk.\n");
			}
			else
			{
				//meaning not local
				
				//First we will send the new directory listings for the new file
				char *currSerialization = serializeQ(overallQ, key, DONT_ADD_DATA, NOT_SET, NOT_SET);
				//printf("currSerialization:\n%s\n", currSerialization);
				
				//create temporary connections
				int currSock = createSocketClient(tempC1->ip_addr, tempC1->port);
				sendMFilesQ(currSerialization, currSock, PUT_DIR_FROM_MASTER);
				
				//Next we will send the mChunks that actually contain the data
				tempC1->data_flag = DATA_IS_SET;
				tempC2->data_flag = DATA_IS_SET;
				
				char *serializedChunk1, *serializedChunk2;
				
				asprintf(&serializedChunk1, "%s#%d#%d#%d#%s#%d#%d#%d#%s%d\r\n\r\n", key, tempC1->chunkID, tempC1->chunkSize, tempC1->chunkType, tempC1->ip_addr, tempC1->port, tempC1->total_chunks, tempC1->data_flag, tempC1->data, PUT_CHUNK_FROM_MASTER);
				
				asprintf(&serializedChunk2, "%s#%d#%d#%d#%s#%d#%d#%d#%s%d\r\n\r\n", key, tempC2->chunkID, tempC2->chunkSize, tempC2->chunkType, tempC2->ip_addr, tempC2->port, tempC2->total_chunks, tempC2->data_flag, tempC2->data, PUT_CHUNK_FROM_MASTER);
				
				int sendSock1 = createSocketClient(tempC1->ip_addr, tempC1->port);
				int sendSock2 = createSocketClient(tempC2->ip_addr, tempC2->port);
				
				send(sendSock1, serializedChunk1, strlen(serializedChunk1), 0);
				send(sendSock2, serializedChunk2, strlen(serializedChunk2), 0);
				
				free(serializedChunk1);
				free(serializedChunk2);
			}
		}
		else
		{
			fprintf(stderr, "FATAL Error from multicastMFilesQeueue. Improperly formatted overallQ\n");
			return;
		}
	}
}

void sendMFilesQ(char *serializedQ, int tSocket, int payload_action)
{
	char *msgToSend;
	asprintf(&msgToSend, "%s%d\r\n\r\n", serializedQ, payload_action);
	send(tSocket, msgToSend, strlen(msgToSend), 0);
	free(msgToSend);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: joinNetwork
 * @param	: None
 * @return	: Always 0
 * @purpose	: Join the network
 ******************************************************************************/
int joinNetwork()
{
	//first we connect to contactMachine (setup the socket)
	masterMachineInfo->socket = createSocketClient(masterMachineInfo->master_IP, masterMachineInfo->master_port);
	if(masterMachineInfo->socket == -1)
	{
		fprintf(stderr, "createSocketClient error: %d\n", masterMachineInfo->socket);
		exit(1);
	}
	
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	sendPayload(mPayload, masterMachineInfo->socket, JOIN);
	destroyPayload(mPayload);
	
	//get response
	char *mResponse = receiveResponse(masterMachineInfo->socket);
	
	//parse the response
	queue_init(&mServerML);
	deserializeMembershipList(&mServerML, mResponse);
	mprintf("Recieved Membership List from master server\n");
	printMembershipList(&mServerML, "mServer's Membership List");
	
        int sizeofmlp = 0;
        pthread_mutex_lock(&mList_mutex);
        sizeofmlp = queue_size(&mServerML);
        pthread_mutex_unlock(&mList_mutex);
        if (sizeofmlp != 2)
	    getMasterDir();//if sizeofmlp == 2, master needs to change dir and send it to this server
	return 0;
}

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: leaveNetwork
 * @param	: None
 * @return	: Always 0
 * @purpose	: Voluntarily Leave the Network
 ******************************************************************************/
int leaveNetwork()
{
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	multicastMsg(&mServerML, mPayload, LEAVE, 0);
	destroyPayload(mPayload);
	mprintf("Leaving Network\n");
	return 0;
}


int getMasterDir()
{
	//It is assumed this function will only be called from joinNetwork
	//meaning that the conenction to the masterMachine is already established
	
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	sendPayload(mPayload, masterMachineInfo->socket, GET_MASTER_DIR);
	destroyPayload(mPayload);
	
	//get response
	char *mResponse = receiveResponse(masterMachineInfo->socket);
	
	if(strcasecmp(mResponse, "ZERO\r\n\r\n") == 0)
	{
		//meaning the master directory has zero entries.
		//We do nothing in this case.
		mprintf("Received Empty Directory from master Server\n");
	}
	else
	{
		deserializeDir(mResponse, &mLocalDir, &mKeys);
		mprintf("Received Directory from master server\n");
	}
	
	return 0;
}


void mDestructor(int call_from)
{
	printDir(&mLocalDir, &mKeys);
	
	if(call_from == GENERAL)
	{
		if(mLocalIP)
			free(mLocalIP);
		
		if(mDNSInfo)
		{
			if(mDNSInfo->mDNS_IP)
				free(mDNSInfo->mDNS_IP);
			free(mDNSInfo);
		}
		
		if(masterMachineInfo)
		{
			if(masterMachineInfo->master_IP)
				free(masterMachineInfo->master_IP);
			free(masterMachineInfo);
		}
	}
	
	unsigned int i;
	for(i=0; i<queue_size(&mBlockQueue); i++)
	{
		mBlock_t *currBlock = (mBlock_t *)queue_at(&mBlockQueue, i);
		if(currBlock->socket != SOCKET_NOT_SET)
			shutdown(currBlock->socket, SHUT_RDWR);
		
		free(currBlock);
	}
	queue_destroy(&mBlockQueue);
	
	//Maybe the following needs to go under GENERAL instead of CALL_FROM_mDISPATCHER
	//Remember to not double free (i.e. values in mKeys queue
	// are pointing to the same thing as the keys in mLocalDir).
	mCustomDestroyDictionary(&mLocalDir, &mKeys);
	mCustomDestroyKeys(&mKeys);
}



void mCustomDestroyKeys(queue_t *incomingQ)
{
	unsigned int i;
	for(i=0; i<queue_size(incomingQ); i++)
	{
		char *value = (char *)queue_at(incomingQ, i);
		if(value)
			free(value);
	}
	queue_destroy(incomingQ);
}


void mCustomDestroyDictionary(dictionary_t *incomingD, queue_t *incomingKeys)
{
	unsigned int i;
	for(i=0; i<queue_size(incomingKeys); i++)
	{
		char *key = queue_at(incomingKeys, i);
		printf("destroying key: %s\n", key);
		queue_t *tempQ = (queue_t *)dictionary_get(incomingD, key);
		deallocateMFile(tempQ); //destroys Q as well
	}
	dictionary_destroy(incomingD);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: mSignalHandler
 * @param	: signal is the caught signal
 * @return	: void
 * @purpose	: Custom signal handler for handling SIGINT
 ******************************************************************************/
void mSignalHandler(int signal)
{
	printf("Caught SIGINT signal --> reached mSignalHandler: %d\n", signal);
	leaveNetwork();
	mDestructor(GENERAL);
	destroy_mLoggingSystem();
	//destoryHeartbeatSystem();
	exit(1);
}


void  changeleader(){//call when leader fail or leave
	pthread_mutex_lock(ml_mutex);
	mPayload_t * leader = (mPayload_t *)queue_at(&mServerML,0);
	pthread_mutex_unlock(ml_mutex);
	if(masterMachineInfo->master_IP)
		free(masterMachineInfo->master_IP);
	asprintf(&masterMachineInfo->master_IP, "%s", leader->IP_address);
	masterMachineInfo->master_port = leader->port;
	masterMachineInfo->socket = SOCKET_NOT_SET;
	mprintf("Master Machine Resolved --> %s:%d\n", masterMachineInfo->master_IP, masterMachineInfo->master_port);
}



void test1()
{
	
	queue_t *splitQ = mSplit("testfile", 4, DISK);
	
	unsigned int i;
	for(i=0; i<queue_size(splitQ); i++)
	{
		mChunk_t *tChunk = (mChunk_t *)queue_at(splitQ, i);
		char *temp;
		asprintf(&temp, "testfile.chunk.%d", (i+1));
		writeToFile(tChunk->data, tChunk->chunkSize, temp);
		free(temp);
	}
	
	char *combined = mJoin(splitQ); //needs to be freed
	printf("Combined: %s\n", combined);
	writeToFile(combined, strlen(combined), "testfile.combined");
	
	free(combined);
	deallocateMFile(splitQ);
	
	//================ Example for servant mServer
	printf("\n\nExample For mServer Servant\n");
	dictionary_t localDir;
	dictionary_init(&localDir);
	
	queue_t *mChunksQ = mSplit("testfile2", 4, DISK);
	printf("Completed mSplit on testfile2\n");
	
	dictionary_add(&localDir, "testfile2", mChunksQ);
	printf("Added mChunksQ for testfile2 to localDirectory Dictioanry\n");
	
	queue_t *mChunksQ2 = mSplit("testfile3", 4, DISK);
	printf("Completed mSplit on testfile3\n");
	
	dictionary_add(&localDir, "testfile3", mChunksQ2);
	printf("Added mChunksQ2 for testfile3 to localDirectory Dictionary\n");
	
	printf("Now we will de-assemble everything...\n");
	sleep(3);
	
	
	queue_t *test1 = (queue_t *)dictionary_get(&localDir, "testfile2");
	unsigned int i1;
	for(i1 = 0; i1<queue_size(test1); i1++)
	{
		mChunk_t *temp1 = (mChunk_t *)queue_at(test1, i1);
		printf("temp1 chunk %d\n", i1);
		char *fname;
		asprintf(&fname, "testfile2.chunk.%d", (i1+1));
		writeToFile(temp1->data, temp1->chunkSize, fname);
		free(fname);
	}
	
	queue_t *test2 = (queue_t *)dictionary_get(&localDir, "testfile3");
	unsigned int i2;
	for(i2 = 0; i2<queue_size(test2); i2++)
	{
		mChunk_t *temp1 = (mChunk_t *)queue_at(test2, i2);
		printf("temp2 chunk %d\n", i2);
		char *fname;
		asprintf(&fname, "testfile3.chunk.%d", (i2+1));
		writeToFile(temp1->data, temp1->chunkSize, fname);
		free(fname);
	}
	
	char *combinedT1 = mJoin(test1);
	writeToFile(combinedT1, strlen(combinedT1), "testfile2.combined");
	free(combinedT1);
	
	char *combinedT2 = mJoin(test2);
	writeToFile(combinedT2, strlen(combinedT2), "testfile3.combined");
	free(combinedT2);
	
	
	deallocateMFile(mChunksQ); //be carefull on not double freeing!! test1 is same thing
	deallocateMFile(mChunksQ2);
	dictionary_destroy(&localDir);
	
}

void test2()
{
	printf("\n\nTEST2\n\n");
	char *testBuffer;
	asprintf(&testBuffer, "Hello World, this is a testfile that will be split into chunks and then put back together using the mFileLib library.\n");
	queue_t *tempQ = mSplit(testBuffer, 4, BUFFER);
	
	char *combined = mJoin(tempQ);
	printf("Combined[%d]: %s", (int)strlen(combined), (char *)combined);
	
	free(combined);
	deallocateMFile(tempQ);
	free(testBuffer);
}

void test3()
{
	mChunk_t *newChunk = (mChunk_t *)malloc(sizeof(mChunk_t));
	init_mChunk(newChunk);
	char *serialized = serializeMChunk(newChunk, ADD_DATA);
	printf("serialized mChunk::%s::\n", serialized);
	
	destroyMChunk(newChunk);
	
	mChunk_t *dChunk = deserializeMChunk(serialized);
	printMChunk(dChunk);
	
	destroyMChunk(dChunk);
	free(serialized);
	
	
	mChunk_t *newChunk2 = (mChunk_t *)malloc(sizeof(mChunk_t));
	init_mChunk(newChunk2);
	newChunk2->chunkID = 5;
	newChunk2->chunkSize = 45;
	newChunk2->chunkType = ORIGINAL;
	asprintf(&newChunk2->data, "This is the data");
	asprintf(&newChunk2->ip_addr, "192.168.1.2");
	newChunk2->port = 5000;
	
	char *s2 = serializeMChunk(newChunk2, ADD_DATA);
	printf("serialized mChunk2::%s::\n", s2);
	
	destroyMChunk(newChunk2);
	
	mChunk_t *dChunk2 = deserializeMChunk(s2);
	printMChunk(dChunk2);
	free(s2);
	
}

void test4()
{
	dictionary_t *mDictionary = (dictionary_t *)malloc(sizeof(dictionary_t));
	dictionary_init(mDictionary);
	queue_t *theKeys = (queue_t *)malloc(sizeof(queue_t));
	queue_init(theKeys);
	
	
	queue_t *mChunksQ1 = mSplit("testfile", 4, DISK);
	queue_t *mChunksQ2 = mSplit("testfile2", 4, DISK);
	queue_t *mChunksQ3 = mSplit("testfile3", 4, DISK);
	
	char *key1, *key2, *key3;
	asprintf(&key1, "testfile");
	asprintf(&key2, "testfile2");
	asprintf(&key3, "testfile3");
	
	dictionary_add(mDictionary, key1, mChunksQ1);
	dictionary_add(mDictionary, key2, mChunksQ2);
	dictionary_add(mDictionary, key3, mChunksQ3);
	
	queue_enqueue(theKeys, key1);
	queue_enqueue(theKeys, key2);
	queue_enqueue(theKeys, key3);
	
	
	
	
	printDir(mDictionary, theKeys);
	
	printf("nNow We will serialize the dictioanry\n");
	char *serializedD = serializeDir(mDictionary, theKeys);
	//printf("%s", serializedD);
	
	
	printf("Now we will deserialize and print the dictionary\n");
	dictionary_t *deserializedD = (dictionary_t *)malloc(sizeof(dictionary_t));
	dictionary_init(deserializedD);
	queue_t *newKeys = (queue_t *)malloc(sizeof(queue_t));
	queue_init(newKeys);
	
	
	
	
	deserializeDir(serializedD, deserializedD, newKeys);
	printDir(deserializedD, newKeys);
	
	
	
	
	
	mCustomDestroyDictionary(mDictionary, theKeys);
	free(mDictionary);
	
	mCustomDestroyDictionary(deserializedD, newKeys);
	free(deserializedD);
	
	mCustomDestroyKeys(theKeys);
	mCustomDestroyKeys(newKeys);
	
	free(serializedD);
}




