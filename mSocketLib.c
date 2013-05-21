/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mSocketLib.c
 * @purpose		: Socket Library
 ******************************************************************************/
#include "mSocketLib.h"



/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: createSocketServer
 * @param	: Port is port the socket will listen on
 * @return	: socket
 * @purpose	: Performs the Socket, Bind, Listen portions of socket creation
 ******************************************************************************/
int createSocketServer(int port)
{
	int mErr;
	memset(&hints, 0, sizeof(hints)); //zero out the hints struct
	hints.ai_family = AF_UNSPEC; //IPv4 or IPv6? (System decides)
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; //kernel --> localhost IP
	char port_buffer[50];
	sprintf(port_buffer, "%d", port);
	mErr = getaddrinfo(NULL, port_buffer, &hints, &result);
	if(mErr != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(mErr));
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	
	//Now we will do 3 of the 4 step socket creation --> 1. socket; 2. bind; 3. listen
	int mSocket;
	mSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(mSocket == -1)
	{
		fprintf(stderr, "socket error: %s\n", strerror(mSocket));
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	
	int yes = 1;
	mErr = setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
	
	if(mErr == -1)
	{
		perror("setsockopt error\n");
	}
	
	int mBind = bind(mSocket, result->ai_addr, result->ai_addrlen);
	if(mBind == -1)
	{
		fprintf(stderr, "bind error: %s\n", strerror(mBind));
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	
	int mListen = listen(mSocket, BACKLOG);
	if(mListen == -1)
	{
		fprintf(stderr, "listen error: %s\n", strerror(mListen));
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	
	freeaddrinfo(result);
	return mSocket;
}



/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: createSocketClient
 * @param	: Port is port the socket will listen on
 * @return	: socket
 * @purpose	: Performs the client side socket connect
 ******************************************************************************/
int createSocketClient(char *ip_addr, int port)
{
	int sockfd;
	struct hostent *he;
	struct sockaddr_in their_addr;
	he = gethostbyname(ip_addr);
	if(he == NULL)
	{
		fprintf(stderr, "he error\n");
		return -1;
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("socket()");
		return -1;
	}
	
	//host byte order
	their_addr.sin_family = AF_INET;
	//int the_port = atoi(port);
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	//zero out rest the struct
	memset(&(their_addr.sin_zero), '\0', 8);
	
	int mErr = connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
	if(mErr == -1)
	{
		perror("connect()");
		return -1;
	}
	return sockfd;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: socketAccept
 * @param	: incomingSocket is a sucessfully created socket (from createSocket)
 * @return	: socket
 * @purpose	: Performs the accept portion of socket creation
 ******************************************************************************/
int socketAccept(int incomingSocket)
{
	struct sockaddr_storage client_addr;
	socklen_t mAddrLen = sizeof(client_addr);
	int mSocket = accept(incomingSocket, (struct sockaddr *)&client_addr, &mAddrLen);
	if(mSocket == -1)
	{
		fprintf(stderr, "socket accept error: %s\n", strerror(mSocket));
		exit(EXIT_FAILURE);
	}
	return mSocket;
}

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: socketClose
 * @param	: incomingSocket is a sucessfully created socket (from createSocket)
 * @return	: close return value
 * @purpose	: Closes this end of the socket (note other end may still be alive)
 ******************************************************************************/
int socketClose(int incomingSocket)
{
	return close(incomingSocket);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: receiveResponse
 * @param	: Socket to receive on
 * @return	: Received Message
 * @purpose	: Efficient buffering system to receive socket messages
 ******************************************************************************/
char *receiveResponse(int sockfd)
{
	//char *mResponse = "\0";
	char *mResponse = "";
	int flag = 1;
	//char receive_buffer[BUFFER_SIZE];
	
	
	int mTotalReceived = 0;
	int lastPos = 0;
	//memset(mResponse, '\0', 0);
	
	while(1)
	{
		char *receive_buffer = (char *)malloc(BUFFER_SIZE);
		if((strstr(mResponse, "\r\n\r\n")) != NULL)
		{
			//printf("about to break...:%s\n", mOutput);
			free(receive_buffer);
			break;
		}
		//printf("recv...\n");
		int retSize = (int)recv(sockfd, receive_buffer, BUFFER_SIZE, 0);
		if(retSize == -1)
		{
			fprintf(stderr, "recv error\n");
			close(sockfd);
			free(receive_buffer);
			return NULL;
		}
		else if(retSize == 0)
		{
			fprintf(stderr, "Connected machine has closed the connection\n");
			close(sockfd);
			free(receive_buffer);
			return NULL;
		}
		else
		{
			//Method 1
			mTotalReceived += retSize;
			if(flag == 1)
			{
				//first time running, we need to malloc
				mResponse = (char *)malloc(retSize + 1);
				flag = 0;
			}
			else
			{
				//iteration i>1 --> need to realloc
				mResponse = (char *)realloc(mResponse, mTotalReceived + 1);
			}
			//next we strcpy or memmove
			strncpy(&mResponse[lastPos], receive_buffer, retSize);
			lastPos += retSize;
			mResponse[mTotalReceived] = '\0';
			
			
			/*
			//Method 2 --> Theoretically much faster!!!
			mTotalReceived += retSize;
			if(flag == 1)
			{
				//first time running
				asprintf(&mResponse, "%s", receive_buffer);
				flag = 0;
			}
			else
			{
				//iteration i>1
				char *temp = mResponse;
				asprintf(&mResponse, "%s%s", mResponse, receive_buffer);
				free(temp);
			}
			mResponse[mTotalReceived] = '\0';
			*/

						
			free(receive_buffer);
		}
	}
	
	return mResponse;
}


