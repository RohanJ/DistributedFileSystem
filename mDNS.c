/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mDNS.c
 * @purpose		: Fixed DNS Server
 ******************************************************************************/
#include "mDNS.h"

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Input Error: Must supply port\n");
		return -1;
	}
	
	//First we extract the port and check if its a valid port
	mLocalPort = atoi(argv[1]);
	if(mLocalPort <= 0 || mLocalPort >= 65536)
	{
		fprintf(stderr, "Invalid Port Number");
		return -1;
	}
	
	//Next we get the local ip, and initialize the logging system and masterMachineInfo
	char *localIP = getLocalIP();
	init_mLoggingSystem(localIP, mLocalPort);
	printf("mDNS running on IP Address/Port: %s:%d\n", localIP, mLocalPort);
	free(localIP);
	
	masterMachineInfo = (masterMachineInfo_t *)malloc(sizeof(masterMachineInfo_t));
	masterMachineInfo->master_IP = NULL;
	masterMachineInfo->master_port = -1;
	mprintf("There is no leader currently elected\n");
	
	createSignalHandler();
	
	//Next start monitoring the network for leader election results
	int mSocketListen = createSocketServer(mLocalPort);
	queue_init(&mBlockQueue);
	mprintf("Now monitoring the network for leader...\n");
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
		pthread_create(&newThread, NULL, mResolve_thread, (void *)newBlock);
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


void createSignalHandler()
{
	struct sigaction customSignal;
	customSignal.sa_handler = mSignalHandler;
	customSignal.sa_flags = 0;
	sigemptyset(&customSignal.sa_mask);
	sigaction(SIGINT, &customSignal, NULL);
}

void *mResolve_thread(void *arg)
{
	mBlock_t *mBlock = (mBlock_t *)arg;
	int mSocket = mBlock->socket;
	//we don't need the while loop here becasue this is a non-persistent connection.
	char *mSerializedMsg = receiveResponse(mSocket);
	if(mSerializedMsg == NULL)
	{
		printf("FATAL error, mResolve_thread returning NULL\n");
		return NULL;
	}
	
	//next we parse the request
	int payloadAction = getPayloadAction(mSerializedMsg);
	mPayload_t *mPayload = deserializePayload(mSerializedMsg);
	
	if(payloadAction == VOTE)
	{
		//first we check if current masterMachineInfo is NULL. If so, we set incoming
		//machine as the master and respond back accordingly. If masterMachineInfo exists
		//then we return masterMachineInfo.
		pthread_mutex_lock(&mMI_mutex);
		if(masterMachineInfo->master_IP == NULL)
		{
			
			asprintf(&masterMachineInfo->master_IP, "%s", mPayload->IP_address);
			masterMachineInfo->master_port = mPayload->port;
			mprintf("Leader Elected --> %s:%d\n", mPayload->IP_address, mPayload->port);
		}
		
		mPayload_t *tPayload = generatePayload(masterMachineInfo->master_IP, masterMachineInfo->master_port);
		sendPayload(tPayload, mSocket, NOACTION);
		destroyPayload(tPayload);
		pthread_mutex_unlock(&mMI_mutex);
	}
	else if(payloadAction == CLIENT)
	{
		pthread_mutex_lock(&mMI_mutex);
		mPayload_t *tPayload;
		if(masterMachineInfo->master_IP == NULL)
			tPayload = generatePayload("", 0);
		else
			tPayload = generatePayload(masterMachineInfo->master_IP, masterMachineInfo->master_port);
		
		sendPayload(tPayload, mSocket, NOACTION);
		destroyPayload(tPayload);
		pthread_mutex_unlock(&mMI_mutex);
	}
	else if(payloadAction == FAILURE)
	{
		//incoming ip_Addr and port is of the newly elected leader
		pthread_mutex_lock(&mMI_mutex);
		if(masterMachineInfo->master_IP)
			free(masterMachineInfo->master_IP);
		asprintf(&masterMachineInfo->master_IP, "%s", mPayload->IP_address);
		masterMachineInfo->master_port = mPayload->port;
		mprintf("Leader Elected --> %s:%d\n", mPayload->IP_address, mPayload->port);
		pthread_mutex_unlock(&mMI_mutex);
	}
	else
	{
		printf("Should Never Reach here --> incorrect payloadAction: %d", payloadAction);
	}
	
	
	destroyPayload(mPayload);
	return NULL;
}


void mDestructor()
{
	if(masterMachineInfo)
	{
		if(masterMachineInfo->master_IP)
			free(masterMachineInfo->master_IP);
		free(masterMachineInfo);
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
	
}


void mSignalHandler(int signal)
{
	printf("Caught SIGINT signal --> reached mSignalHandler: %d\n", signal);
	mDestructor();
	destroy_mLoggingSystem();
	exit(1);
}
