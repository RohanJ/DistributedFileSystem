/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mLoggingLib.h
 * @purpose		: Logging Library Header File
 ******************************************************************************/
#ifndef mLoggingLib_h
#define mLoggingLib_h

#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void init_mLoggingSystem(char *ip_addr, int port);
char *mLogging_ip_addr; //must be set by mNode
int mLogging_port; //must be set by mNode
void destroy_mLoggingSystem();

void mprintf(char *param,...);
void mputs(char *str);
void mputc(char c); 
char *convert(unsigned int num, int base);
void writeToLog(char *msg);

char *mOutput;

char *generateTimestamp();


#define DECIMAL 10
#define OCTAL 8
#define HEX 16

#endif