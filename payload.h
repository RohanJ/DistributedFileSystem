/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: payload.h
 * @purpose		: Payload Library Header File
 ******************************************************************************/
#ifndef payload_h
#define payload_h

#define _GNU_SOURCE

#include "queue.h"
#include "libdictionary.h"
#include "mFileLib.h"
#include "mSocketLib.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef struct _mPayload_t
{
	time_t hb_timestamp;
	char *timestamp;
	char *IP_address;
	int port;
	int socket;
	int seq_num;
	
	int udp_socket;
}mPayload_t;

#define SOCKET_UNSPECIFIED -1
#define SEQ_NUM_UNSPECIFIED -1
#define NOT_SET -1
#define IS_SET 1
#define COLUMN_SIZE 256
#define PAYLOAD_SIZE 1024

//Payload Action Definitions
#define JOIN 10
#define LEAVE 11
#define FAILURE 12
#define NOACTION 13
#define HEARTBEAT 14
#define VOTE 15
#define PUT_FROM_CLIENT 16
#define PUT_DIR_FROM_MASTER 17
#define PUT_CHUNK_FROM_MASTER 18
#define GET_FROM_CLIENT 19
#define GET_FROM_MASTER 20
#define CLIENT 21
#define GET_MASTER_DIR 22
#define FILE_NOT_FOUND 23
#define FILE_FOUND 24
#define FAILURE_SHIFT 25
#define DELETE_FROM_CLIENT 26
#define DELETE_FROM_MASTER 27

#define PA_SIZE 2 //payloadAction size
#define ME_DELIM_SIZE 4 //message end delimiter size

//===============NOTES
//use time(&rawtime) and then
//timeinfo = localtime(&rawtime) to set the timeinfo in the struct
//then on the recieving side, use strftime(buffer, 80, "%a %b %Y [%m-%d-%Y] (%z) %I:%M%p", timeinfo);

//=====Payload Element Specific
mPayload_t *generatePayload(char *ip_addr, int port);
int sendPayload(mPayload_t *incomingPayload, int tSocket, int paylaodAction);
char *serializePayload(mPayload_t *incomingPayload, int payloadAction);
mPayload_t *deserializePayload(char *serializedMsg);
int getPayloadAction(char *serializedMsg);
void destroyPayload(mPayload_t *incomingPayload);
void printPayload(mPayload_t *incomingPayload, char *opt_message);

//=====Membership List Specific
void addToML(queue_t *incomingML, mPayload_t *incomingPayload);
void removeFromML(queue_t *incomingML, mPayload_t *incomingPayload);
char *serializeMembershipList(queue_t *incomingML);
void deserializeMembershipList(queue_t *incomingML, char *serializedML);
void destroyMembershipList(queue_t *incomingML);
void printMembershipList(queue_t *incomingML, char *opt_message);

//=====Local Directory Specific
void addToDir(dictionary_t *incomingD, char *key, mChunk_t *incomingChunk, queue_t *tKeys);
void removeFromDir(dictionary_t *incomingD, char *key, mChunk_t *incomingChunk, queue_t *tKeys);
void printDir(dictionary_t *incomingD, queue_t *tKeys);
char *serializeMChunk(mChunk_t *incomingMChunk, int data_flag);
#define DONT_ADD_DATA 0
#define ADD_DATA 1

mChunk_t *deserializeMChunk(char *serializedMChunk);
char *serializeQ(queue_t *incomingQ, char *key, int indexing_flag, int index1, int index2);
#define ADD_ALL_DATA 1
#define CUSTOM_INDEXING 2

char *extractKeyFromSerializedQ(char *serializedQ);
void deserializeQ(char *serializedQ, char *key, dictionary_t *incomingD, queue_t *tKeys);
char *serializeDir(dictionary_t *incomingD, queue_t *tKeys);
void deserializeDir(char *serializedDir, dictionary_t *incomingD, queue_t *tKeys);
int sendDir(dictionary_t *incomingDir);
void sendChunk(mChunk_t *tChunk, char *sdfsName, char *clientIP, int clientPort);

//=====Multicasting
void multicastMsg(queue_t *incomingML, mPayload_t *incomingPayload, int payloadAction, int currSocket);

int existsinML(queue_t *ML, mPayload_t *Payload);


#endif