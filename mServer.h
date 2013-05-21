/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mServer.h
 * @purpose		: Main Servers for SDFS (Header File)
 ******************************************************************************/
#ifndef mServer_h
#define mServer_h

#define _GNU_SOURCE

#include "queue.h"
#include "payload.h"
#include "mSocketLib.h"
#include "mLoggingLib.h"
#include "mFileLib.h"
#include "libdictionary.h"
#include "failureleader.h"

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

//Following struct is not used if mServerType = MASTER
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
queue_t mServerML; //mServer's membership list
dictionary_t mLocalDir; //Local File Directory
pthread_mutex_t mList_mutex = PTHREAD_MUTEX_INITIALIZER;
//Note that we do not need a mutex for mLocalDir because the dictionary library
//is thread safe.
queue_t mKeys; //need to store keys used in dict for use in mDestructor

char *mLocalIP;
int mLocalPort;
int mServerType;
#define MASTER 1
#define SERVANT 2

void extractArgs(char *argv[]);
void createSignalHandler();
char *getLocalIP();
int resolveMaster();

void mDispatcher(); //decides functionality based on mServerType
#define THREAD_RUNNING 1
#define THREAD_NOT_RUNNING 2

//Master/Servant network monitoring related:
pthread_t monitorNetwork_thread;
int mThreadState;
void *monitorNetwork(void *arg);


//failure detection and heartbeat
pthread_t monitor_heartbeat_thread;
void *monitorHeartbeats(void *arg);

int joinNetwork();
int leaveNetwork();

int getMasterDir();

//Distribution/Replication Related
void mDistributionScheme(mPayload_t *inPyld);
int mReplicationScheme(int split_factor, int list_index);
void multicastMFilesQueue(queue_t *overallQ, char *key, int replication_factor);
void sendMFilesQ(char *serializedQ, int tSocket, int payload_action);

queue_t *getLocalFilesList(queue_t *incomingQ, int restrict_flag);
#define NO_RESTRICT -1

void mDeleteFile(dictionary_t *incomingD, queue_t *incomingKeys, char *filename);

void mDestructor(int call_from);
#define CALL_FROM_mDISPATCHER 1
#define GENERAL 2
void mCustomDestroyKeys(queue_t *incomingQ);
void mCustomDestroyDictionary(dictionary_t *incomingD, queue_t *incomingKeys);

void mSignalHandler(int signal);
void  changeleader();
void makeSecondReplica();
void test1();
void test2();
void test3();
void test4();

#endif
