/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: mLoggingLib.c
 * @purpose		: Logging Library
 ******************************************************************************/
#include "mLoggingLib.h"


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: init_mLoggingSystem
 * @param	: IP Address and port
 * @return	: void
 * @purpose	: Initialize Logging Library
 ******************************************************************************/
void init_mLoggingSystem(char *ip_addr, int port)
{
	asprintf(&mLogging_ip_addr, "%s", ip_addr);
	mLogging_port = port;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: destroy_mLoggingSystem
 * @param	: None
 * @return	: void
 * @purpose	: Deallocate memory associated with the logging system
 ******************************************************************************/
void destroy_mLoggingSystem()
{
	if(mLogging_ip_addr)
		free(mLogging_ip_addr);
	if(mOutput)
		free(mOutput);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: mprintf
 * @param	: printf style formatted msg
 * @return	: void
 * @purpose	: Custom re-implementation of printf to allow for conveniently 
 *				printing to screen and writing to log file
 ******************************************************************************/
void mprintf(char *param,...)
{
	//we write to stdout
	char *str_ptr, *param_ptr;
	int i;
	unsigned u;
	
	va_list argp;
	va_start(argp, param);
	param_ptr = param;
	
	for(param_ptr = param; *param_ptr != '\0'; param_ptr++)
	{
		if(*param_ptr != '%')
		{
			putchar(*param_ptr);
			mputc(*param_ptr);
			continue;
		}
		
		param_ptr++;
		if(*param_ptr == 'c')
		{
			i = va_arg(argp, int);
			putchar(i);
			mputc(*param_ptr);
		}
		else if(*param_ptr == 'd')
		{
			i=va_arg(argp, int);
			if(i < 0)
			{
				i = -i;
				putchar('-');
				mputc(*param_ptr);
			}
			mputs(convert(i,DECIMAL));
		}
		else if(*param_ptr == 'o')
		{
			i=va_arg(argp, unsigned int);
			mputs(convert(i, OCTAL));
		}
		else if(*param_ptr == 's')
		{
			str_ptr=va_arg(argp,char *);
			mputs(str_ptr);
		}
		else if(*param_ptr == 'u')
		{
			u = va_arg(argp, unsigned int);
			mputs(convert(u, DECIMAL));
		}
		else if(*param_ptr == 'x')
		{
			u = va_arg(argp, unsigned int);
			mputs(convert(u, HEX));
		}
		else if(*param_ptr == '%')
		{
			putchar('%');
			mputc(*param_ptr);
		}
		else
		{
			mputs("[UNDEFINED_SYMBOL]");
		}
	}//end for loop
	va_end(argp);
	

	//we write to logfile
	//printf("mOutput: %s", mOutput);
	writeToLog(mOutput);
	free(mOutput);
	mOutput = NULL;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: mputs
 * @param	: str
 * @return	: void
 * @purpose	: put string on screen and wrtie string to overall mOutput
 ******************************************************************************/
void mputs(char *str)
{
	int i;
	for(i=0; i<(int)strlen(str); i++)
	{
		putchar(str[i]);
		mputc(str[i]);
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: mputc
 * @param	: char c
 * @return	: void
 * @purpose	: put char on screen and wrtie char to overall mOutput
 ******************************************************************************/
void mputc(char c)
{
	if(mOutput != NULL)
	{
		char *temp = mOutput;
		asprintf(&mOutput, "%s%c", mOutput, c);
		free(temp);
	}
	else
	{
		asprintf(&mOutput, "%c", c);
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: convert
 * @param	: num, base
 * @return	: string containing num is specified base
 * @purpose	: Convert given number to specified base
 ******************************************************************************/
char *convert(unsigned int num, int base)
{
	static char buffer[33];
	char *ptr;
	ptr=&buffer[sizeof(buffer)-1];
	*ptr='\0';
	do
	{
		*--ptr="0123456789abcdef"[num%base];
		num/=base;
	}while(num!=0);
	return(ptr);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: writeToLog
 * @param	: message to write to log file
 * @return	: void
 * @purpose	: Write emssage to log file
 ******************************************************************************/
void writeToLog(char *msg)
{
	//printf("about to write to log file: %s", msg);
	FILE *fw;
	char *fName;
	asprintf(&fName, "%s_%d.log", mLogging_ip_addr, mLogging_port);
	if((fw=fopen(fName,"a"))==NULL)
	{
		fprintf(stderr,"Error: Could not create log file\n");
		return;
	}
	char *timestamp = generateTimestamp();
	fprintf(fw,"%s: %s", timestamp, msg);
	fclose(fw);
	free(fName);
	free(timestamp);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: generateTimestamp
 * @param	: None
 * @return	: String containing timestamp
 * @purpose	: Generate timestamp from lcoaltime
 ******************************************************************************/
char *generateTimestamp()
{
	time_t rawtime;
	time(&rawtime);
	char time_buffer[80];
	strftime(time_buffer, 80, "[%m/%d/%y]%I:%M:%S", localtime(&rawtime));
	char *allocd_buffer;
	asprintf(&allocd_buffer, "%s", time_buffer);
	return allocd_buffer;
}


