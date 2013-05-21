#ifndef failureleader_h
#define failureleader_h

#define _GNU_SOURCE

#include "queue.h"
#include "payload.h"
#include "mSocketLib.h"
#include "mLoggingLib.h"
#include "mFileLib.h"
#include "libdictionary.h"


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

#define HEARTBEATTIME 1
#define CHECKINGTIME 2
#define FAILURETIME 3L


pthread_t alive;
pthread_t checking;
int mUdpsock;

char* myIP;
int myPort;
dictionary_t* myDic;
queue_t* myKeys;
pthread_mutex_t* ml_mutex;
char * mDNSIP;
int mDNSport;
int pre;
int suc;
void initHeartbeatSystem(queue_t *incomingML,char* myip, int myport,dictionary_t* mLocalDir,
                 queue_t* mKeys,pthread_mutex_t* mList_mutex,char* mdnsip, int mdnsp);
int receiveUdpmsg(queue_t *mlp);
int failorleave(int flag, mPayload_t *temp,int founder,queue_t *mlp);
void *checkfunc(void *arg);
void *livefunc(void *arg);
void sendUDPmessage(char *ip, int port, int sockfd, mPayload_t * payload, int payloadaction);
void updatetimestamp(char* buf,queue_t *mlp);
int createbindUDPSocket(int udpport);
int createUDPSocket();
void destoryHeartbeatSystem();
int findPredecessor(queue_t *mlp, char* ip, int port,int detect);
int findSuccessor(queue_t *mlp, char* ip, int port);
void leaderhelper(queue_t *mlp);
void handlereplicas(char* ip, int port,char* preip, int preport, char* sucip,int sucport,char* prepreip, int prepreport, char* sucsucip,int sucsucport);
void alloriginal(char* ip, int port);
#endif
