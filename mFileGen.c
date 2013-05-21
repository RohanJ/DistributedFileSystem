/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mFileLib.h
 * @purpose		: File Generator for Testing Purposes
 ******************************************************************************/
#ifndef mFileGen_h
#define mFileGen_h

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>



#endif

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "Input Error. Arguments must be as follows:\n");
		fprintf(stderr, "argv[1] will be Filename to output to\n");
		fprintf(stderr, "argv[2] will be File Size in MBs\n");
		return -1;
	}
	
	char *filename = argv[1];
	int fileSize = atoi(argv[2]);
	
	int total_bytes_written = 0;
	int total_bytes_needed = 1024 * 1024 * fileSize;
	
	
	//open file for binary writing
	FILE *mFile = fopen(filename, "wb");
	if(mFile == NULL)
	{
		fprintf(stderr, "File Cannot be opened/created\n");
		exit(1);
	}

	
	int line_num = 0;
	while(total_bytes_written < total_bytes_needed)
	{
		char *tempStr;
		asprintf(&tempStr, "This is a testfile testline for testing purposes. Line %d.\n", line_num);
		fwrite(tempStr, 1, strlen(tempStr), mFile);
		total_bytes_written += strlen(tempStr);
		line_num++;
	}
	
	fclose(mFile);
	printf("%d MB File: %s created.\n", fileSize, filename);
	
	return 0;
}