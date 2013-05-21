/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mSocketLib.h
 * @purpose		: Socket Library Header File
 ******************************************************************************/
#ifndef mSocketLib_h
#define mSocketLib_h

#define _GNU_SOURCE

#include "queue.h"
#include "payload.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#define BACKLOG 10
#define BUFFER_SIZE 1024 * 1024 * 10 //10MB

struct addrinfo hints, *result;

int createSocketServer(int port);
int createSocketClient(char *ip_addr, int port);
int socketAccept(int incomingSocket);
int socketClose(int incomingSocket);

char *receiveResponse(int sockfd);


#endif