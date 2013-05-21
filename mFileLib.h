/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mFileLib.h
 * @purpose		: File IO Header File
 ******************************************************************************/
#ifndef mFileLib_h
#define mFileLib_h

#define _GNU_SOURCE

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
#define CEILING_NEG(X) ((X-(int)(X)) < 0 ? (int)(X-1) : (int)(X))
#define CEILING(X) ( ((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X) )

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "queue.h"
#include "libdictionary.h"

typedef struct _mChunk_t
{
	int chunkID;
	int chunkSize;
	int chunkType;
	char *ip_addr;
	int port;
	int total_chunks;
	int data_flag;
	char *data;
}mChunk_t;

#define ORIGINAL 1
#define REPLICA 2
//queue_t mChunkQueue;

#define DATA_NOT_SET 0
#define DATA_IS_SET 1

queue_t *mSplit(char *filename, int split_factor, int location);
//for mSplit location:
#define DISK 1
#define BUFFER 2


void init_mChunk(mChunk_t *incomingChunk);
#define NOT_SET -1

void *mJoin(queue_t *inQ);
long getFileSize(FILE *incomingFile);
void assignSplitSizes(queue_t *inQ, int split_factor, long fSize);
void readFromFile(queue_t *inQ, FILE *mFile);
void readFromFileToMChunk(mChunk_t *tChunk, FILE *mFile);
void readFromBuffer(queue_t *inQ, char *tBuffer);
int writeToFile(void *tData, size_t tsize, char *filename);

mChunk_t *newCopy_mChunk(mChunk_t *incomingChunk);

void printMChunk(mChunk_t *incomingChunk);
void printChunksQueue(queue_t *incomingQ);
void destroyMChunk(mChunk_t *incomingChunk);
void deallocateMFile(queue_t *inQ);

#endif