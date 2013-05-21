/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mDNS.h
 * @purpose		: Fixed DNS Server (Header File)
 ******************************************************************************/
#ifndef mDNS_h
#define mDNS_h

#define _GNU_SOURCE

#include "queue.h"
#include "payload.h"
#include "mSocketLib.h"
#include "mLoggingLib.h"

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

typedef struct _masterMachineInfo_t
{
	char *master_IP;
	int master_port;
}masterMachineInfo_t;
masterMachineInfo_t *masterMachineInfo;
pthread_mutex_t mMI_mutex = PTHREAD_MUTEX_INITIALIZER; //Mutex for read/write on m.M.I

typedef struct _mBlock_t
{
	pthread_t tid;
	int socket;
}mBlock_t;

#define SOCKET_NOT_SET -1
queue_t mBlockQueue;


int mLocalPort;
char *getLocalIP();
void createSignalHandler();
void *mResolve_thread(void *arg);
void mDestructor();
void mSignalHandler(int signal);


#endif