/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mFileLib.c
 * @purpose		: File IO Library
 ******************************************************************************/
#include "mFileLib.h"

void init_mChunk(mChunk_t *incomingChunk)
{
	incomingChunk->chunkID = NOT_SET;
	incomingChunk->chunkSize = NOT_SET;
	incomingChunk->chunkType = NOT_SET;
	incomingChunk->ip_addr = NULL;
	incomingChunk->port = NOT_SET;
	incomingChunk->total_chunks = NOT_SET;
	incomingChunk->data_flag = DATA_NOT_SET;
	incomingChunk->data = NULL;
	
}

queue_t *mSplit(char *filename, int split_factor, int location)
{
	queue_t *mChunkQueue = (queue_t *)malloc(sizeof(queue_t));
	queue_init(mChunkQueue);
	
	
	if(location == DISK)
	{
		FILE *mFile = fopen(filename, "rb"); //read in binary mode
		if(mFile == NULL)
		{
			fprintf(stderr, "File cannot be read\n");
			exit(1);
		}
	
		long mFileSize = getFileSize(mFile);
		//printf("The File Size is: %ld\n", mFileSize);
		
		assignSplitSizes(mChunkQueue, split_factor, mFileSize);
		
		readFromFile(mChunkQueue, mFile);
		fclose(mFile);
	}
	else if(location == BUFFER)
	{
		long mBufferSize = (long)strlen(filename);
		//printf("The Buffer Size is: %ld\n", mBufferSize);
		assignSplitSizes(mChunkQueue, split_factor, mBufferSize);
		readFromBuffer(mChunkQueue, filename); //filename is the buffer in this case
	}
	else
	{
		fprintf(stderr, "Invalid location\n");
		queue_destroy(mChunkQueue);
		exit(1);
	}
	
	return mChunkQueue;
}


void *mJoin(queue_t *inQ)
{
	char *mCombined="", *temp = NULL;
	unsigned int i;
	for(i=0; i<queue_size(inQ); i++)
	{
		mChunk_t *tChunk = (mChunk_t *)queue_at(inQ, i);
		//printf("mJoin: [ID %d]--|Size %d --> %s\n", tChunk->chunkID, tChunk->chunkSize, (char *)tChunk->data);
		
		temp = mCombined;
		asprintf(&mCombined, "%s%s", mCombined, (char *)tChunk->data);
		if(i != 0)
			free(temp);
	}
	return mCombined;
}


long getFileSize(FILE *incomingFile)
{
	fseek(incomingFile, 0, SEEK_END);
	long tSize = ftell(incomingFile);
	rewind(incomingFile);
	return tSize;
}


void assignSplitSizes(queue_t *inQ, int split_factor, long fSize)
{
	int split_size = CEILING(fSize / (double)split_factor);
	int excludingLastSize = (split_factor-1) * split_size;
	
	int i;
	for(i=0; i<split_factor; i++)
	{
		mChunk_t *newChunk = (mChunk_t *)malloc(sizeof(mChunk_t)); //must eventually be freed (probably after file is sent over)
		init_mChunk(newChunk);
		newChunk->chunkID = i;
		newChunk->total_chunks = split_factor;
		if(i == split_factor - 1)
			newChunk->chunkSize = ((int)fSize - excludingLastSize);
		else
			newChunk->chunkSize = split_size;
		
		queue_enqueue(inQ, newChunk);
	}
}

void readFromFile(queue_t *inQ, FILE *mFile)
{
	unsigned int i;
	for(i=0; i<queue_size(inQ); i++)
	{
		mChunk_t *tChunk = (mChunk_t *)queue_at(inQ, i);
		//printf("mSplit: [ID %d]-->Size %d\n", tChunk->chunkID, tChunk->chunkSize);
		tChunk->data = (char *)malloc( (tChunk->chunkSize * sizeof(char)) + 1);
		size_t result = fread(tChunk->data, 1, tChunk->chunkSize, mFile);
		
		//next we must null terminate because fread does not do so.
		char *temp = tChunk->data;
		temp[tChunk->chunkSize] = '\0';
		
		if((int)result != tChunk->chunkSize)
		{
			printf("Reading Error\n");
			fclose(mFile);
			exit(1);
		}
		tChunk->data_flag = DATA_IS_SET;
	}
}

void readFromFileToMChunk(mChunk_t *tChunk, FILE *mFile)
{
	tChunk->data = (char *)malloc( (tChunk->chunkSize * sizeof(char)) + 1);
	size_t result = fread(tChunk->data, 1, tChunk->chunkSize, mFile);
	
	//next we must null terminate because fread does not do so.
	char *temp = tChunk->data;
	temp[tChunk->chunkSize] = '\0';
	
	if((int)result != tChunk->chunkSize)
	{
		printf("Reading Error\n");
		fclose(mFile);
		exit(1);
	}
	tChunk->data_flag = DATA_IS_SET;

}

void readFromBuffer(queue_t *inQ, char *tBuffer)
{
	int bytes_copied = 0;
	unsigned int i;
	for(i=0; i<queue_size(inQ); i++)
	{
		mChunk_t *tChunk = (mChunk_t *)queue_at(inQ, i);
		//printf("mSplit: [ID %d]-->Size %d\n", tChunk->chunkID, tChunk->chunkSize);
		tChunk->data = (char *)malloc( (tChunk->chunkSize * sizeof(char)) + 1);
		strncpy(tChunk->data, tBuffer+bytes_copied, tChunk->chunkSize);
		bytes_copied += tChunk->chunkSize;
		
		//We must null terminate because strncpy does not do so
		char *temp = tChunk->data;
		temp[tChunk->chunkSize] = '\0';
		
		tChunk->data_flag = DATA_IS_SET;
	}
}


int writeToFile(void *tData, size_t tsize, char *filename)
{
	FILE *fw;
	if((fw=fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "Error: Could not write to %s\n", filename);
		return -1;
	}
	//else we continue and write to the file
	fwrite(tData, 1, tsize, fw);
	fclose(fw);
	return 0;
}


mChunk_t *newCopy_mChunk(mChunk_t *incomingChunk)
{
	//NOTE this will not copy data, because its inefficient and unnecessary
	mChunk_t *newChunk = (mChunk_t *)malloc(sizeof(mChunk_t));
	init_mChunk(newChunk);
	newChunk->chunkID = incomingChunk->chunkID;
	newChunk->chunkSize = incomingChunk->chunkSize;
	newChunk->total_chunks = incomingChunk->total_chunks;

	return newChunk;
}


void printMChunk(mChunk_t *incomingChunk)
{
	printf("==mChunk ID: %d\n", incomingChunk->chunkID);
	printf("Size: %d\n", incomingChunk->chunkSize);
	if(incomingChunk->chunkType == ORIGINAL) printf("Type: ORIGINAL\n");
	else if(incomingChunk->chunkType == REPLICA) printf("Type: REPLICA\n");
	else printf("Type: N/A\n");
	
	
	if(incomingChunk->ip_addr) printf("IP: %s\n", incomingChunk->ip_addr);
	else printf("IP: N/A\n");
	
	if(incomingChunk->port != NOT_SET) printf("Port: %d\n", incomingChunk->port);
	else printf("Port: N/A\n");
	
	if(incomingChunk->total_chunks != NOT_SET) printf("Total Chunks: %d\n", incomingChunk->total_chunks);
	else printf("Total Chunks: N/A\n");
	
	if(incomingChunk->data) printf("Data: %s\n", (char *)incomingChunk->data);
	else printf("Data: NULL\n");
	
	printf("==========\n\n");
}

void printChunksQueue(queue_t *incomingQ)
{
	unsigned int j;
	for(j=0; j<queue_size(incomingQ); j++)
	{
		mChunk_t *currChunk = (mChunk_t *)queue_at(incomingQ, j);
		printMChunk(currChunk);
	}
}


void destroyMChunk(mChunk_t *incomingChunk)
{
	//we only free if ORIGINAL. REPLICA just has pointers to ORIGINAL data
	if(incomingChunk->chunkType == ORIGINAL  && incomingChunk->data)
		free(incomingChunk->data);
	if(incomingChunk->ip_addr)
		free(incomingChunk->ip_addr);
	free(incomingChunk);
}


void deallocateMFile(queue_t *inQ)
{
	unsigned int i;
	for(i=0; i<queue_size(inQ); i++)
	{
		mChunk_t *currChunk = (mChunk_t *)queue_at(inQ, i);
		destroyMChunk(currChunk);
	}
	queue_destroy(inQ);
	free(inQ);
}
