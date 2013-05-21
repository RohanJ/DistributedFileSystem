/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mClient.c
 * @purpose		: Client for SDFS
 ******************************************************************************/
#include "mClient.h"

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		fprintf(stderr, "Input Error. Arguments must be as follows:\n");
		fprintf(stderr, "argv[1] will be mDNS ip address\n");
		fprintf(stderr, "argv[2] will be mDNS port\n");
		fprintf(stderr, "argv[3] will be mClient's port to run on\n");
		return -1;
	}
	
	extractArgs(argv);
	init_mLoggingSystem(mLocalIP, mLocalPort);
	//resolveMaster();
	
	printf("==========COMMAND LIST==========\n");
	printf("| put localfilename sdfsfilename|\n");
	printf("| get sdfsfilename localfilename|\n");
	printf("| delete sdfsfilename force     |\n");
	printf("| exit                          |\n");
	printf("|_______________________________|\n");
	
		
	//Next we proactively launch the recv threads
	//The logic is that the probability of a FILE_FOUND is much higher than FILE_NOT_FOUND
	mSocketListen = createSocketServer(mLocalPort);
	launchDispatcher();
	
	
	while(1)
	{
		printf("Enter command: \n");
		fflush(stdin);
		fflush(stdout);
		//The following is buffer overflow safe.
		size_t nbytes = 256;
		char *getLineBuffer;
		getline(&getLineBuffer, &nbytes, stdin);
		
		resolveMaster();
		
		char *input;
		asprintf(&input, "%s", getLineBuffer);
		parse_exec(input);
		free(input);
	}
	
	return 0;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: extractArgs
 * @param	: argv
 * @return	: void
 * @purpose	: Extract and set accordingly args passed to mClient
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
	sendPayload(mPayload, mDNSInfo->socket, CLIENT);
	destroyPayload(mPayload);
	
	//get response
	char *mResponse = receiveResponse(mDNSInfo->socket);
	
	//parse the response
	mPayload_t *masterPayload = deserializePayload(mResponse);
	
	
	
	if(strcmp(masterPayload->IP_address,"") == 0)
	{
		mprintf("Network Down. No Available Master\n");
		mDestructor(CALL_FROM_EXIT);
		exit(1);
	}
	
	if(masterMachineInfo->master_IP != NULL)
		free(masterMachineInfo->master_IP);
	
	asprintf(&masterMachineInfo->master_IP, "%s", masterPayload->IP_address);
	masterMachineInfo->master_port = masterPayload->port;
	mprintf("Master Machine Resolved --> %s:%d\n", masterMachineInfo->master_IP, masterMachineInfo->master_port);
	
	return 0;
}

void launchDispatcher()
{
	queue_init(&mBlockQueue);
	mBlock_t *dispatcherBlock = (mBlock_t *)malloc(sizeof(mBlock_t));
	dispatcherBlock->tid = dispatcherThread;
	dispatcherBlock->socket = SOCKET_NOT_SET;
	queue_enqueue(&mBlockQueue, dispatcherBlock);
	numChunks = -1; //dispatcher exit clause
	pthread_create(&dispatcherThread, NULL, mDispatcher, NULL);
}

void parse_exec(char *input)
{
	input[strlen(input) - 1] = '\0'; //strip \n
	
	if(strcasecmp(input, "exit") == 0)
	{
		mDestructor(CALL_FROM_EXIT);
		free(input);
		exit(1);
	}
	
	char delimiter = ' ';
	int brkpt1=-1, brkpt2=-1;
	
	int currPos;
	for(currPos=0; currPos<(int)strlen(input); currPos++)
	{
		if(brkpt1 == -1 && input[currPos] == delimiter)
		{
			brkpt1 = currPos;
			continue;
		}
		if(brkpt2 == -1 && input[currPos] == delimiter)
		{
			brkpt2 = currPos;
			continue;
		}
	}
	
	if(brkpt1 == -1 || brkpt2 == -1)
	{
		printf("Invalid Input\n");
		return;
	}
	
	char *command = (char *)malloc(brkpt1 + 1);
	strncpy(command, input, brkpt1);
	command[brkpt1] = '\0';
	
	if(strcasecmp(command, "put") != 0 && strcasecmp(command, "get") != 0 && strcasecmp(command, "delete") != 0)
	{
		printf("Invalid Command\n");
		free(command);
		return;
	}
	else
	{
		char *arg1, *arg2;
		arg1 = (char *)malloc(brkpt2 - brkpt1);
		strncpy(arg1, input + brkpt1 + 1, brkpt2 - brkpt1 -1);
		arg1[brkpt2-brkpt1-1] = '\0';
		
		arg2 = (char *)malloc(strlen(input) - brkpt2 + 1);
		strncpy(arg2, &input[brkpt2 + 1], strlen(input) - brkpt2);
		arg2[strlen(arg2)] = '\0';
		
		if(strcasecmp(command, "put") == 0)
			put(arg1, arg2); //arg1 and arg2 are freed from within put an get
		else if(strcasecmp(command, "get") == 0)
			get(arg1, arg2);
		else
			deleteFile(arg1, arg2);
		
		free(command);
	}
}


void put(char *localfilename, char *sdfsfilename)
{
	time_t start, end;
	start = clock();
	
	masterMachineInfo->socket = createSocketClient(masterMachineInfo->master_IP, masterMachineInfo->master_port);
	if(masterMachineInfo->socket == -1)
	{
		fprintf(stderr, "createSocketClient error: %d\n", masterMachineInfo->socket);
		exit(1);
	}
	
	
	//next we read the file and send the file
	FILE *mFile = fopen(localfilename, "rb"); //read in binary mode
	if(mFile == NULL)
	{
		fprintf(stderr, "File cannot be read\n");
		exit(1);
	}
	
	long mFileSize = getFileSize(mFile);
	char *mBuffer = (char *)malloc( (mFileSize * sizeof(char)) + 1);
	size_t result = fread(mBuffer, 1, mFileSize, mFile);
	mBuffer[mFileSize] = '\0';
	
	if((long)result != mFileSize)
	{
		printf("Reading Error\n");
		exit(1);
	}
	
	//mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	//Hack: The server doesn't care who the sender is, so ip_addr is sdfsfilename
	//server doesn't care about timestamp, so timestamp is mBuffer
	mPayload_t *mPayload = generatePayload(sdfsfilename, mLocalPort);
	free(mPayload->timestamp);
	mPayload->timestamp = mBuffer;
	sendPayload(mPayload, masterMachineInfo->socket, PUT_FROM_CLIENT);
	destroyPayload(mPayload);
	//WARNING DO NOT free mBuffer, becauase of hack mentioned above, destroyPayload
	//takes care of this.
	
	
	fclose(mFile);
	free(localfilename);
	free(sdfsfilename);
	
	end = clock();
	printf("Task PUT finished in: %lf\n", (float)(end-start)/CLOCKS_PER_SEC);
}


void get(char *sdfsfilename, char *localfilename)
{
	time_t start, end;
	start = clock();
	
	masterMachineInfo->socket = createSocketClient(masterMachineInfo->master_IP, masterMachineInfo->master_port);
	if(masterMachineInfo->socket == -1)
	{
		fprintf(stderr, "createSocketClient error: %d\n", masterMachineInfo->socket);
		exit(1);
	}
	
	//Hack: The server does care who the sender is, but timestamp is irrelevant.
	//So timestamp is the filename (sdfsfilename)
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	free(mPayload->timestamp);
	//mPayload->timestamp = sdfsfilename; //cant just do pointers cuz destroyPayload will free it
	asprintf(&mPayload->timestamp, "%s", sdfsfilename);
	sendPayload(mPayload, masterMachineInfo->socket, GET_FROM_CLIENT);
	destroyPayload(mPayload);
	
	
	//next we wait for response
	char *mResponse = receiveResponse(masterMachineInfo->socket);
	if(mResponse == NULL)
	{
		printf("mClient recieved NULL in get. Returning...\n");
		free(sdfsfilename);
		free(localfilename);
		return;
	}
	
	//parse the response
	int payloadAction = getPayloadAction(mResponse);
	if(payloadAction == FILE_NOT_FOUND)
	{
		mprintf("File Not Found.\n");
	}
	else if(payloadAction == FILE_FOUND)
	{
		mResponse[strlen(mResponse) - (ME_DELIM_SIZE + PA_SIZE)] = '\0';
		int num_chunks = atoi(mResponse);
		mprintf("File Found --> %d chunks. Download initiating...\n", num_chunks);
		

		//Next we wait until all downloads are finished (Note that downloads are in parallel)
		pthread_join(dispatcherThread, NULL);
		mprintf("Finished Downloading File: %s.\n", sdfsfilename);
		
		//Reorder tempQ 
		queue_t *mFilesQ = (queue_t *)malloc(sizeof(queue_t));
		queue_init(mFilesQ);
		int i;
		for(i=0; i<numChunks; i++)
		{
			char tKey[16];
			sprintf(tKey, "%d", i);
			int theIndex = atoi((char *)dictionary_get(&indexingD, tKey));
			queue_enqueue(mFilesQ, queue_at(&tempQ, theIndex));
		}
		//printChunksQueue(mFilesQ);
		
		//and then writeToFile
		char *combined = mJoin(mFilesQ);
		writeToFile(combined, strlen(combined), localfilename);
		
		
		//must destroy tempQ, indexingD (and Deallocate key and value) then IMPORTANT --> must init them again!!
		mDestructor(CALL_FROM_GET);
		queue_init(&tempQ);
		dictionary_init(&indexingD);
		
		//must destroy mFilesQ
		deallocateMFile(mFilesQ);
		
		//then we must relaunch dispatcher
		launchDispatcher();
	}
	else
	{
		printf("mClient recv unrecognized payloadAction: %d\n", payloadAction);
	}
		
	
	
	free(mResponse);
	free(sdfsfilename);
	free(localfilename);
	
	end = clock();
	printf("Task GET finished in: %lf\n", (float)(end-start)/CLOCKS_PER_SEC);
}


void deleteFile(char *sdfsfilename, char *option)
{
	masterMachineInfo->socket = createSocketClient(masterMachineInfo->master_IP, masterMachineInfo->master_port);
	if(masterMachineInfo->socket == -1)
	{
		fprintf(stderr, "createSocketClient error: %d\n", masterMachineInfo->socket);
		exit(1);
	}
	
	//next we generate the payload with according payload action and send
	//Hack: The server doesn't care about timestamp, so that will be the filename
	mPayload_t *mPayload = generatePayload(mLocalIP, mLocalPort);
	free(mPayload->timestamp);
	mPayload->timestamp = sdfsfilename;
	sendPayload(mPayload, masterMachineInfo->socket, DELETE_FROM_CLIENT);
	destroyPayload(mPayload);
	//WARNING DO NOT free sdfsfilename because of hack mentioned above, destroyPayload takes care of this
	
	printf("%s Deleted file.\n", option);
	free(option);
}


void *mDispatcher(void *arg)
{
	if(arg == NULL)
	{}
	

	queue_init(&tempQ);
	dictionary_init(&indexingD);
	
	int threads_launched = 0;
	while (1)
	{
		mBlock_t *newBlock = (mBlock_t *)malloc(sizeof(mBlock_t));
		newBlock->socket = SOCKET_NOT_SET;
		queue_enqueue(&mBlockQueue, newBlock);
		
		//printf("threads_launched: %d and numChunks: %d\n", threads_launched, numChunks);
		//Exit clause
		pthread_mutex_lock(&numChunks_mutex);
		int mutex_protected_value = numChunks;
		pthread_mutex_unlock(&numChunks_mutex);
		if(threads_launched == mutex_protected_value)
		{
			//here we should wait for sub threads to finish
			unsigned int i;
			for(i=0; i<queue_size(&mBlockQueue); i++)
			{
				mBlock_t *mBlock = (mBlock_t *)queue_at(&mBlockQueue, i);
				if(pthread_equal(mBlock->tid, pthread_self()) != 0)
				{
					//printf("Waiting on tid:%lu\n", (unsigned long)mBlock->tid);
					pthread_join(mBlock->tid, NULL);
				}
			}
			
			return NULL;
		}
		
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
		pthread_create(&newThread, NULL, mDownloadThread, (void *)newBlock);
		threads_launched++;
		//printf("Launched thread: %lu\n", (unsigned long) newThread);
		pthread_mutex_lock(&numChunks_mutex);
	}
	return NULL;
}


void *mDownloadThread(void *arg)
{
	mBlock_t *mBlock = (mBlock_t *)arg;
	int mSocket =  mBlock->socket;
	
	char *mSerializedMsg = receiveResponse(mSocket);
	if(mSerializedMsg == NULL)
	{
		printf("FATAL ERROR: get receving NULL\n");
		exit(EXIT_FAILURE);
	}
	
	//Next we parse the request
	mChunk_t *tChunk = deserializeMChunk(mSerializedMsg);

	numChunks = tChunk->total_chunks; //idempotent
	pthread_mutex_unlock(&numChunks_mutex);
	
	pthread_mutex_lock(&tempQ_mutex);
	int currIndex = (int)queue_size(&tempQ);
	char *tKey;
	asprintf(&tKey, "%d", tChunk->chunkID);
	
	char *tValue;
	asprintf(&tValue, "%d", currIndex);
	dictionary_add(&indexingD, tKey, tValue);
	
	queue_enqueue(&tempQ, tChunk);
	pthread_mutex_unlock(&tempQ_mutex);
	
	return NULL;
}

void mDestructor(int CALL_FROM)
{
	if(CALL_FROM == CALL_FROM_EXIT)
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
		
		destroy_mLoggingSystem();
	}
	
	unsigned int i;
	for(i=0; i<queue_size(&mBlockQueue); i++)
	{
		mBlock_t *currBlock = (mBlock_t *)queue_at(&mBlockQueue, i);
		//if(CALL_FROM != CALL_FROM_GET)
		//	pthread_kill(currBlock->tid, SIGKILL);
		if(currBlock->socket != SOCKET_NOT_SET)
			shutdown(currBlock->socket, SHUT_RDWR);
		free(currBlock);
	}
	queue_destroy(&mBlockQueue);
	
	queue_destroy(&tempQ);

	int j;
	for(j=0; j<numChunks; j++)
	{
		char tKey[16];
		sprintf(tKey, "%d", j);
		dictionary_remove_free(&indexingD, tKey);
	}
	dictionary_destroy(&indexingD);
}