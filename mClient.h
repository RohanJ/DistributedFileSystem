/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mClient.h
 * @purpose		: Client for SDFS (Header File)
 ******************************************************************************/
#ifndef mClient_h
#define mClient_h

#define _GNU_SOURCE

#include "queue.h"
#include "payload.h"
#include "mSocketLib.h"
#include "mLoggingLib.h"
#include "mFileLib.h"

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

typedef struct mDNSInfo_t
{
	char *mDNS_IP;
	int mDNS_port;
	int socket;
}mDNSInfo_t;
mDNSInfo_t *mDNSInfo;

typedef struct _masterMachineInfo_t
{
	char *master_IP;
	int master_port;
	int socket;
}masterMachineInfo_t;
masterMachineInfo_t *masterMachineInfo;

typedef struct _mBlock_t
{
	pthread_t tid;
	int socket;
}mBlock_t;

#define SOCKET_NOT_SET -1
queue_t mBlockQueue;


char *mLocalIP;
int mLocalPort;

void extractArgs(char *argv[]);
char *getLocalIP();
int resolveMaster();
void parse_exec(char *input);
void put(char *localfilename, char *sdfsfilename);
void get(char *sdfsfilename, char *localfilename);
void deleteFile(char *sdfsfilename, char *option);

pthread_t dispatcherThread;
void launchDispatcher();
void *mDispatcher(void *arg);
void *mDownloadThread(void *arg);
int mSocketListen;

int numChunks; //needs mutex
pthread_mutex_t numChunks_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t tempQ_mutex = PTHREAD_MUTEX_INITIALIZER;
queue_t tempQ;
dictionary_t indexingD;
#define NOT_FINISHED 0
#define FINISHED 1

void mDestructor(int CALL_FROM);
#define CALL_FROM_GET 1
#define CALL_FROM_EXIT 2

#endif
