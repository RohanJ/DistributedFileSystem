/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: payload.c
 * @purpose		: Payload Library
 ******************************************************************************/
#include "payload.h"
#include "libdictionary.h"


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: generatePayload
 * @param	: IP Address and port
 * @return	: mPayload struct
 * @purpose	: Generate a payload with specified ip address and port, initialized
 *				with current timestamp
 ******************************************************************************/
mPayload_t *generatePayload(char *ip_addr, int port)
{
	mPayload_t *mPayload = (mPayload_t *)malloc(PAYLOAD_SIZE);
	
	time_t rawtime;
	time(&rawtime);
	char time_buffer[80];
	strftime(time_buffer, 80, "[%m/%d/%y]%I:%M:%S", localtime(&rawtime));
	asprintf(&mPayload->timestamp, "%s", time_buffer);
	
	asprintf(&mPayload->IP_address, "%s", ip_addr);
	mPayload->port = port;
	
	
	
	mPayload->socket = SOCKET_UNSPECIFIED;
	mPayload->seq_num = SEQ_NUM_UNSPECIFIED;
	return mPayload;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: sendPayload
 * @param	: Payload to send, socket it to send it on, action assoc. w/ payload
 * @return	: Always 0.
 * @purpose	: Send a payload on specified socket with specified action
 ******************************************************************************/
int sendPayload(mPayload_t *incomingPayload, int tSocket, int payloadAction)
{
	char *serializedMsg = serializePayload(incomingPayload, payloadAction);
	
	if(payloadAction == PUT_FROM_CLIENT)
	{
		printf("Sending...\n");
	}
	
	send(tSocket, serializedMsg, strlen(serializedMsg), 0);
	
	if(payloadAction == PUT_FROM_CLIENT)
		printf("Finished sending.\n");
	
	free(serializedMsg);
	return 0;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: serializePayload
 * @param	: Payload to serialize, acction asso. w/ paylaod
 * @return	: Serialized string version of payload
 * @purpose	: Serialize a payload for sending across sockets
 ******************************************************************************/
char *serializePayload(mPayload_t *incomingPayload, int payloadAction)
{
	char *serializedMsg;
	if(incomingPayload->socket == SOCKET_UNSPECIFIED && incomingPayload->seq_num == SEQ_NUM_UNSPECIFIED)
		asprintf(&serializedMsg, "%s#%d#%s%d\r\n\r\n", incomingPayload->IP_address, incomingPayload->port, incomingPayload->timestamp, payloadAction);
	else
		asprintf(&serializedMsg, "%s#%d#%s#%d#%d%d\r\n\r\n", incomingPayload->IP_address, incomingPayload->port, incomingPayload->timestamp, incomingPayload->socket, incomingPayload->seq_num, payloadAction);
	
	return serializedMsg;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: deserializePayload
 * @param	: Serialized payload
 * @return	: Payload struct
 * @purpose	: Convert serialized msg recv on socket to a payload struct
 ******************************************************************************/
mPayload_t *deserializePayload(char *serializedMsg)
{
	int pA = getPayloadAction(serializedMsg);
	
	//Frst we must remove \r\n\r\n delimiter (-4) and the payloadAction (-2)
	serializedMsg[strlen(serializedMsg) - (ME_DELIM_SIZE + PA_SIZE)] = '\0';
	int brkpt1=-1, brkpt2=-1, brkpt3=-1, brkpt4=-1;
	
	int currPos;
	for(currPos=0; currPos<(int)strlen(serializedMsg); currPos++)
	{
		if(brkpt1 == -1 && serializedMsg[currPos] == '#')
		{
			brkpt1 = currPos;
			continue;
		}
		if(brkpt2 == -1 && serializedMsg[currPos] == '#')
		{
			brkpt2 = currPos;
			if(pA == PUT_FROM_CLIENT || pA == FAILURE_SHIFT)
			{
				break; //because we do not need to scan any further
			}
			else
				continue;
		}
		if(brkpt3 == -1 && serializedMsg[currPos] == '#')
		{
			brkpt3 = currPos;
			continue;
		}
		if(brkpt4 == -1 && serializedMsg[currPos] == '#')
		{
			brkpt4 = currPos;
			continue;
		}
	}
	
	
	//so if brkpt 3 and 4 are -1, means socket_unspecified and seq_number_unspecified
	char *ip_address = (char *)malloc(brkpt1 + 1);
	strncpy(ip_address, serializedMsg, brkpt1);
	ip_address[brkpt1] = '\0';
	
	char *port = (char *)malloc(brkpt2 - brkpt1);
	strncpy(port, serializedMsg + brkpt1 + 1, brkpt2 - brkpt1 -1);
	port[brkpt2-brkpt1-1] = '\0';
	
	char *timestamp = NULL, *socket= NULL, *seq_num = NULL;
	if(brkpt3 == -1 || brkpt4 == -1)
	{
		//this means that we are now at the last element of the string
		timestamp = (char *)malloc(strlen(serializedMsg) - brkpt2 + 1);
		strncpy(timestamp, &serializedMsg[brkpt2 +1], strlen(serializedMsg) - brkpt2);
		timestamp[strlen(serializedMsg) - brkpt2] = '\0';
	}
	else
	{
		//meaning that socket and seq_num are specified (note that they are mutually exclusive)
		timestamp = (char *)malloc(brkpt3 - brkpt2);
		strncpy(timestamp, serializedMsg + brkpt2 + 1, brkpt3 - brkpt2 - 1);
		timestamp[brkpt3-brkpt2-1] = '\0';
		
		socket = (char *)malloc(brkpt4 - brkpt3);
		strncpy(socket, serializedMsg + brkpt3 + 1, brkpt4 - brkpt3 -1);
		socket[brkpt4-brkpt3-1] = '\0';
		
		seq_num = (char *)malloc(strlen(serializedMsg) - brkpt4 + 1);
		strncpy(seq_num, &serializedMsg[brkpt4 + 1], strlen(serializedMsg) - brkpt4);
		seq_num[strlen(serializedMsg) - brkpt4] = '\0';
	}
	
	//reach here indicating that data has been extracted into strings. Now we create the
	//payload from extracted data.
	mPayload_t *deserializedPayload = (mPayload_t *)malloc(sizeof(mPayload_t));
	asprintf(&deserializedPayload->IP_address, "%s", ip_address);
	deserializedPayload->port = atoi(port);
	asprintf(&deserializedPayload->timestamp, "%s", timestamp);
	if(socket == NULL)
		deserializedPayload->socket = SOCKET_UNSPECIFIED;
	else
		deserializedPayload->socket = atoi(socket);
	if(seq_num == NULL)
		deserializedPayload->seq_num = SEQ_NUM_UNSPECIFIED;
	else
		deserializedPayload->seq_num = atoi(seq_num);
	
	
	free(ip_address);
	free(port);
	free(timestamp);
	if(socket) free(socket);
	if(seq_num) free(seq_num);
	
	return deserializedPayload;	
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: getPayloadAction
 * @param	: Serialized message recv on socket
 * @return	: Action asso. w\ payload
 * @purpose	: To extract the payload action (JOIN, LEAVE, FAILURE)
 ******************************************************************************/
int getPayloadAction(char *serializedMsg)
{
	char buffer[PA_SIZE + 1];
	strncpy(buffer, serializedMsg + (strlen(serializedMsg) - (ME_DELIM_SIZE + PA_SIZE)), PA_SIZE);
	buffer[PA_SIZE] = '\0';
	return atoi(buffer);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: destroyPayload
 * @param	: Payload to destroy
 * @return	: void
 * @purpose	: Deallocate memory associated with the payload
 ******************************************************************************/
void destroyPayload(mPayload_t *incomingPayload)
{
	if(incomingPayload)
	{
		if(incomingPayload->IP_address) free(incomingPayload->IP_address);
		if(incomingPayload->timestamp) free(incomingPayload->timestamp);
		free(incomingPayload);
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: printPayload
 * @param	: Payload to print, optional message to print
 * @return	: void
 * @purpose	: Print pyaload params
 ******************************************************************************/
void printPayload(mPayload_t *incomingPayload, char *opt_message)
{
	printf("%s\n", opt_message);
	printf("|==============PAYLOAD==============|\n");
	printf("|IP Address: %s\n", incomingPayload->IP_address);
	printf("|Port: %d\n", incomingPayload->port);
	printf("|Timestamp: %s\n", incomingPayload->timestamp);
	if(incomingPayload->socket != SOCKET_UNSPECIFIED)
		printf("|Socket: %d\n", incomingPayload->socket);
	if(incomingPayload->seq_num != SEQ_NUM_UNSPECIFIED)
		printf("|Seq_num: %d\n", incomingPayload->seq_num);
	printf("|===================================|\n");
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: addToML
 * @param	: Membership List to add to, Payload to add
 * @return	: void
 * @purpose	: Add a payload if it is non-existant in the memebrship list
 ******************************************************************************/
void addToML(queue_t *incomingML, mPayload_t *incomingPayload)
{	
	if(queue_size(incomingML) == 0)
	{
		//time(&incomingPayload->hb_timestamp);
		incomingPayload->udp_socket = -1;
		queue_enqueue(incomingML, (void *)incomingPayload);
	}
	else
	{
		//Recall that it is the IP address combined with the port that is unique
		char *ip_addr = incomingPayload->IP_address;
		int tPort = incomingPayload->port;
		
		unsigned int i;
		for(i=0; i<queue_size(incomingML); i++)
		{
			mPayload_t *tempPayload = (mPayload_t *)queue_at(incomingML, i);
			if((strcmp(ip_addr, tempPayload->IP_address) == 0) && (tPort == tempPayload->port))
				return; //meaning payload already exists in ML
		}
		
		//reach here indicating that payload not in ML
		//time(&incomingPayload->hb_timestamp);
		incomingPayload->udp_socket = -1;
		queue_enqueue(incomingML, (void *)incomingPayload);
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: removeFromML
 * @param	: Membership Lis to remove from, payload to remove
 * @return	: void
 * @purpose	: Remove payload from membership list
 ******************************************************************************/
void removeFromML(queue_t *incomingML, mPayload_t *incomingPayload)
{
	if(queue_size(incomingML) == 0) return;
	else
	{
		char *ip_addr = incomingPayload->IP_address;
		int tPort = incomingPayload->port;
		
		unsigned int i;
		for(i=0; i<queue_size(incomingML); i++)
		{
			mPayload_t *tempPayload = (mPayload_t *)queue_at(incomingML, i);
			if((strcmp(ip_addr, tempPayload->IP_address) == 0) && (tPort == tempPayload->port))
			{
				queue_remove_at(incomingML, i);
				return;
			}
		}
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: serializeMmbershipList
 * @param	: Membership List to serialize
 * @return	: Serialzed string of membership list
 * @purpose	: Serialize a membership list for sending on sockets
 ******************************************************************************/
char *serializeMembershipList(queue_t *incomingML)
{
	char *serializedML = "", *temp = NULL;
	int set_once_flag = NOT_SET;
	unsigned int i;
	
	for(i=0; i<queue_size(incomingML); i++)
	{
		mPayload_t *tPayload = (mPayload_t *)queue_at(incomingML, i);
		char *tSerializedMsg = serializePayload(tPayload, NOACTION); //needs to be freed
		//remove /r/n/r/n delimiter and payloadAction
		tSerializedMsg[strlen(tSerializedMsg) - (ME_DELIM_SIZE + PA_SIZE)] = '\0';
		
		if(set_once_flag == NOT_SET)
		{
			asprintf(&serializedML, "%s", tSerializedMsg);
			free(tSerializedMsg);
			set_once_flag = IS_SET;
		}
		else
		{
			temp = serializedML;
			asprintf(&serializedML, "%s$%s", serializedML, tSerializedMsg);
			free(tSerializedMsg);
			free(temp);
		}
	}
	
	//Next we must perform some final touchups including the following:
	//add the /r/n/r/n delimeter for sending
	temp = serializedML;
	asprintf(&serializedML, "%s\r\n\r\n", serializedML);
	free(temp);
	 
	return serializedML;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: deserializeMembershipList
 * @param	: Membership List to add to if necessary, Serialzed recv ML
 * @return	: void
 * @purpose	: Deserialized recv ML and add to specified ML if necessary
 ******************************************************************************/
void deserializeMembershipList(queue_t *incomingML, char *serializedML)
{
	//remove /r/n/r/n delimiter and payloadAction
	serializedML[strlen(serializedML) - (ME_DELIM_SIZE + PA_SIZE)] = '\0';
	int reached_end = NOT_SET;
	int pos, lastBrkpt=0;
	while(reached_end == NOT_SET)
	{
		for(pos=lastBrkpt; pos < (int)strlen(serializedML); pos++)
		{
			if(pos == (int)strlen(serializedML) - 1)
			{
				reached_end = IS_SET;
				//this is the case for last payload in series, or
				//if there is only one payload in ML
				
				char *tPayload;
				asprintf(&tPayload, "%s\r\n\r\n", serializedML+lastBrkpt);
				mPayload_t *aPayload = (mPayload_t *)deserializePayload(tPayload); //payload to add
				addToML(incomingML, aPayload);
				free(tPayload);
				break;
			}
			else
			{
				if(serializedML[pos] == '$')
				{
					char *tPayload = (char *)malloc(pos-lastBrkpt +1);
					strncpy(tPayload, serializedML + lastBrkpt, pos-lastBrkpt);
					tPayload[pos - lastBrkpt] = '\0';
					
					char *tPayload_delimited;
					asprintf(&tPayload_delimited, "%s\r\n\r\n", tPayload);
					mPayload_t *aPayload = (mPayload_t *)deserializePayload(tPayload_delimited); //payload to add
					addToML(incomingML, aPayload);
					lastBrkpt = pos + 1;
					free(tPayload);
					free(tPayload_delimited);
					break;
				}
			}
		}
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: destroyMembershipList
 * @param	: Membership List to destory
 * @return	: void
 * @purpose	: Deallocate memory associated with a membership list
 ******************************************************************************/
void destroyMembershipList(queue_t *incomingML)
{
	unsigned int i;
	for(i=0; i<queue_size(incomingML); i++)
	{
		mPayload_t *tempPayload = (mPayload_t *)queue_at(incomingML, i);
		destroyPayload(tempPayload);
	}
	queue_destroy(incomingML);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: printMembershipList
 * @param	: Membership List to print, optional message to print
 * @return	: void
 * @purpose	: Print assoc. params of membership list
 ******************************************************************************/
void printMembershipList(queue_t *incomingML, char *opt_message)
{
	printf("%s\n", opt_message);
	unsigned int i;
	for(i=0; i<queue_size(incomingML); i++)
	{
		mPayload_t *tPayload = (mPayload_t *)queue_at(incomingML, i);
		char message[64];
		sprintf(message, "Payload[%d]---------->", i);
		printPayload(tPayload, message);
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: multicastMsg
 * @param	: Mmbership List to send, payload to send, send action, socket of 
 *				mNode or mContactMachine that invoked the function
 * @return	: void
 * @purpose	: Multicast a JOIN, LEAVE, or FAILURE msg
 ******************************************************************************/
void multicastMsg(queue_t *incomingML, mPayload_t *incomingPayload, int payloadAction, int currSocket)
{
	//to the current socket we send the msg
	//to every other socket we serialize the incoming payload with the payload action and send that
	char *serializedPayload = serializePayload(incomingPayload, payloadAction);
	
	if(payloadAction == JOIN)
	{
		char *serializedML = serializeMembershipList(incomingML);
		//first we send the current socket the entire membershiplist
		send(currSocket, serializedML, strlen(serializedML), 0);
		
		//next we send every other socket!=currSocket in the ML the payload
		unsigned int i;
		for(i=0; i<queue_size(incomingML); i++)
		{
			mPayload_t *tPayload = (mPayload_t *)queue_at(incomingML, i);
			if(tPayload->socket != SOCKET_UNSPECIFIED && tPayload->socket != currSocket)
			{
				int currSock = createSocketClient(tPayload->IP_address, tPayload->port);
				send(currSock, serializedPayload, strlen(serializedPayload), 0);
			}
		}
	}
	else if(payloadAction == LEAVE || payloadAction == FAILURE)
	{
		//create temporary connections between other members of the network and multicast
		unsigned int i;
		for(i=0; i<queue_size(incomingML); i++)
		{
			mPayload_t *currPayload = (mPayload_t *)queue_at(incomingML, i);
			if(strcmp(currPayload->IP_address, incomingPayload->IP_address) == 0 && currPayload->port == incomingPayload->port)
				continue;
			else
			{
				int currSock= createSocketClient(currPayload->IP_address, currPayload->port);
				send(currSock, serializedPayload, strlen(serializedPayload), 0);
			}
		}
	}
	else
	{
		printf("invalid multicast\n");
	}
}

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: addToDir
 * @param	: Dictionary to add to, corresp. key, mChunk to add, the Keys Q
 * @return	: void
 * @purpose	: Add mChunk to Dictionary (replacing if already exists)
 ******************************************************************************/
void addToDir(dictionary_t *incomingD, char *key, mChunk_t *incomingChunk, queue_t *tKeys)
{
	//printf("addToDir --> recvd incomingChunk: \n");
	//printMChunk(incomingChunk);
	
	queue_t *tQ = (queue_t *)dictionary_get(incomingD, key);
	if(tQ == NULL)
	{
		//meaning no such key exists
		queue_t *newQ = (queue_t *)malloc(sizeof(queue_t));
		queue_init(newQ);
		queue_enqueue(newQ, incomingChunk);
		dictionary_add(incomingD, key, newQ);
		queue_enqueue(tKeys, key);
		
		//printf("=====Added above mChunk\n");
	}
	else
	{
		//we must iterate through to see if corresponding chunkID already exists
		//if so, just return
		unsigned int i;
		for(i=0; i<queue_size(tQ); i++)
		{
			mChunk_t *tempC = (mChunk_t *)queue_at(tQ, i);
			//return only if chunkID and chunkType are the same
			if(tempC->chunkID == incomingChunk->chunkID)
				if(tempC->chunkType == incomingChunk->chunkType)
				{
					//printf("=====DID NOT ADD, RETURNING\n");
					return;
				}
		}
		
		//if not, we simple enqueue to the queue
		queue_enqueue(tQ, incomingChunk);
		
		//printf("=====Added above mChunk\n");
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: addToDir
 * @param	: Dictionary to remove from, corresp. key, mChunk to remove
 * @return	: void
 * @purpose	: Remove mChunk to Dictionary
 ******************************************************************************/
void removeFromDir(dictionary_t *incomingD, char *key, mChunk_t *incomingChunk, queue_t *tKeys)
{
	//if last element, remember to remove the queue and destroy the key from dict
	queue_t *tQ = (queue_t *)dictionary_get(incomingD, key);
	if(tQ == NULL)
	{
		//doesn't exist
		return;
	}
	else
	{
		unsigned int i;
		for(i=0; i<queue_size(tQ); i++)
		{
			mChunk_t *tempC = (mChunk_t *)queue_at(tQ, i);
			if(tempC->chunkID == incomingChunk->chunkID)
				if(tempC->chunkSize == incomingChunk->chunkSize)
					if(tempC->chunkType == incomingChunk->chunkType)
						if(tempC->data == incomingChunk->data)
							if(tempC->ip_addr == incomingChunk->ip_addr)
								if(tempC->port == incomingChunk->port)
								{
									queue_remove_at(tQ, i);
									dictionary_remove(incomingD, key);
									unsigned int j;
									for(j=0; j<queue_size(tKeys); j++)
										if(strcmp(key, (char *)queue_at(tKeys, j)) == 0)
											queue_remove_at(tKeys, j);
									break;
								}
		}
	}
	
	//next we free incomingChunk
	if(incomingChunk->data)
		free(incomingChunk->data);
	if(incomingChunk->ip_addr)
		free(incomingChunk->ip_addr);
	
	free(incomingChunk);
	
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: printDir
 * @param	: Directory to print, Keys assoc with directory
 * @return	: void
 * @purpose	: Print given Directory
 ******************************************************************************/
void printDir(dictionary_t *incomingD, queue_t *tKeys)
{
	printf("==========DIRECTORY==========\n");
	unsigned int i;
	for(i=0; i<queue_size(tKeys); i++)
	{
		char *currKey = (char *)queue_at(tKeys, i);
		queue_t *currQ = (queue_t *)dictionary_get(incomingD, currKey);
		
		printf("=====KEY: %s\n", currKey);
		printChunksQueue(currQ);
	}
}


char *serializeMChunk(mChunk_t *incomingMChunk, int data_flag)
{
	char *serialized;
	if(data_flag == ADD_DATA)
	{
		asprintf(&serialized, "%d#%d#%d#%s#%d#%d#%d#%s\r\n\r\n", incomingMChunk->chunkID, incomingMChunk->chunkSize, incomingMChunk->chunkType, incomingMChunk->ip_addr, incomingMChunk->port, incomingMChunk->total_chunks, incomingMChunk->data_flag, incomingMChunk->data);
	}
	else if(data_flag == DONT_ADD_DATA)
	{
		char *place_holder = "(null)";
		asprintf(&serialized, "%d#%d#%d#%s#%d#%d#%d#%s\r\n\r\n", incomingMChunk->chunkID, incomingMChunk->chunkSize, incomingMChunk->chunkType, incomingMChunk->ip_addr, incomingMChunk->port, incomingMChunk->total_chunks, incomingMChunk->data_flag, place_holder);
	}
	else
	{
		fprintf(stderr, "SerializeMChunk receviced unidentified data_flag: %d\n", data_flag);
	}
	
	return serialized;
}


mChunk_t *deserializeMChunk(char *serializedMChunk)
{
	//First we remove the \r\n\r\n (-4) delimiter
	serializedMChunk[strlen(serializedMChunk) - ME_DELIM_SIZE] = '\0';
	
	int reached_end = NOT_SET;
	int pos, lastBrkpt = 0;
	mChunk_t *mChunk = (mChunk_t *)malloc(sizeof(mChunk_t));
	init_mChunk(mChunk);
	int element_num = 1;
	
	while(reached_end == NOT_SET)
	{
		for(pos = lastBrkpt; pos < (int)strlen(serializedMChunk); pos++)
		{
			if(serializedMChunk[pos] == '#')
			{
				char *element = (char *)malloc(pos-lastBrkpt + 1);
				strncpy(element, serializedMChunk + lastBrkpt, pos-lastBrkpt);
				element[pos - lastBrkpt] = '\0';
				
				if(element_num == 1)
					mChunk->chunkID = atoi(element);
				else if(element_num == 2)
					mChunk->chunkSize = atoi(element);
				else if(element_num == 3)
					mChunk->chunkType = atoi(element);
				else if(element_num == 4)
				{
					if(strcmp(element, "(null)") != 0)
					{
						asprintf(&mChunk->ip_addr, "%s", element);
					}
				}
				else if(element_num == 5)
				{
					mChunk->port = atoi(element);
				}
				else if(element_num == 6)
				{
					mChunk->total_chunks = atoi(element);
				}
				else if(element_num == 7)
				{
					mChunk->data_flag = atoi(element);
				}
				else
					printf("Theoretically, never reached...\n");
				
				lastBrkpt = pos + 1;
				free(element);
				if(element_num == 7)
					break; //we've reached up to the data
				else
					element_num++;
			}
		} //end for loop
		
		//Now we get the data
		reached_end = IS_SET;
		//this is the case for last element in mChunk which is the data
		if(mChunk->data_flag == DATA_IS_SET)
		{
			int tSize = (int)strlen(serializedMChunk) - lastBrkpt;
			mChunk->data = (char *)malloc(tSize + 1);
			strncpy(mChunk->data, serializedMChunk + lastBrkpt, tSize);
			mChunk->data[tSize] = '\0';
		}
		
	} //end while loop
	return mChunk;
}

void sendChunk(mChunk_t *tChunk, char *sdfsName, char *clientIP, int clientPort)
{
	//first we must read the data from file
	char *filename;
	asprintf(&filename, "[%s:%d]%s_%d", tChunk->ip_addr, tChunk->port, sdfsName, tChunk->chunkID);
	
	FILE *mFile = fopen(filename, "rb");
	readFromFileToMChunk(tChunk, mFile);
	char *serializedChunk = serializeMChunk(tChunk, ADD_DATA);
	
	//next we create connection and send
	int sendSock = createSocketClient(clientIP, clientPort);
	//printf("About to send to [%s:%d] on socket %d\n", clientIP, clientPort, sendSock);
	send(sendSock, serializedChunk, strlen(serializedChunk), 0);
	
	
	//next we must free pointers (even the data pointer inside the chunk)
	free(filename);
	free(serializedChunk);
	free(tChunk->data);
	tChunk->data = NULL;
}


char *serializeQ(queue_t *incomingQ, char *key, int indexing_flag, int index1, int index2)
{
	char *overallS = "", *temp = NULL;
	int set_once_flag = NOT_SET;
	
	unsigned int i;
	for(i=0; i<queue_size(incomingQ); i++)
	{
		mChunk_t *currChunk = (mChunk_t *)queue_at(incomingQ, i);
		char *currS;
		if(indexing_flag == DONT_ADD_DATA)
			currS = serializeMChunk(currChunk, DONT_ADD_DATA);
		else if(indexing_flag == CUSTOM_INDEXING)
		{
			if(index1 != NOT_SET && (int)i == index1)
				currS = serializeMChunk(currChunk, ADD_DATA);
			else if(index2 != NOT_SET && (int)i == index2)
				currS = serializeMChunk(currChunk, ADD_DATA);
			else
				currS = serializeMChunk(currChunk, DONT_ADD_DATA);
		}
		else
		{
			fprintf(stderr, "serializeQ recieved unidentified indexing_flag\n");
		}

			
		//must remove the /r/n/r/n delimiter
		currS[strlen(currS) - ME_DELIM_SIZE] = '\0';
		
		if(set_once_flag == NOT_SET)
		{
			asprintf(&overallS, "%s", currS);
			free(currS);
			set_once_flag = IS_SET;
		}
		else
		{
			temp = overallS;
			asprintf(&overallS, "%s$%s", overallS, currS);
			free(currS);
			free(temp);
		}
	}
	
	//Next we must perform some final touchups
	temp = overallS;
	asprintf(&overallS, "%s#%s\r\n\r\n", key, overallS);
	free(temp);
	
	return overallS;
}


char *extractKeyFromSerializedQ(char *serializedQ)
{
	//remove /r/n/r/n delimiter
	serializedQ[strlen(serializedQ) - ME_DELIM_SIZE] = '\0';
	
	//First we extract the key
	int key_index;
	for(key_index = 0; key_index<(int)strlen(serializedQ); key_index++)
	{
		if(serializedQ[key_index] == '#')
		{
			break;
		}
	}
	char *key = (char *)malloc(key_index + 1);
	strncpy(key, serializedQ, key_index);
	key[key_index] = '\0';
	
	return key;
}

void deserializeQ(char *serializedQ, char *key, dictionary_t *incomingD, queue_t *tKeys)
{
	/*
	//remove /r/n/r/n delimiter
	serializedQ[strlen(serializedQ) - ME_DELIM_SIZE] = '\0';
	
	//First we extract the key
	int key_index;
	for(key_index = 0; key_index<(int)strlen(serializedQ); key_index++)
	{
		if(serializedQ[key_index] == '#')
		{
			break;
		}
	}
	char *key = (char *)malloc(key_index + 1);
	strncpy(key, serializedQ, key_index);
	key[key_index] = '\0';
	
	*/
	
	//Next we parse
	int reached_end = NOT_SET;
	int pos, /*lastBrkpt = key_index + 1; */ lastBrkpt = (int)strlen(key) + 1;
	
	
	while (reached_end == NOT_SET)
	{
		for(pos = lastBrkpt; pos < (int)strlen(serializedQ); pos++)
		{
			if(pos == (int)strlen(serializedQ) - 1)
			{
				reached_end = IS_SET;
				//This is the case for last mChunk in Queue, or
				//if there is only one mChunk in Queue
				
				char *tChunk;
				asprintf(&tChunk, "%s\r\n\r\n", serializedQ + lastBrkpt);
				mChunk_t *aChunk = (mChunk_t *)deserializeMChunk(tChunk);
				
				//straight addToDir
				addToDir(incomingD, key, aChunk, tKeys);
				free(tChunk);
				break;
			}
			else
			{
				if(serializedQ[pos] == '$')
				{
					char *tChunk = (char *)malloc(pos - lastBrkpt + 1);
					strncpy(tChunk, serializedQ + lastBrkpt, pos - lastBrkpt);
					tChunk[pos - lastBrkpt] = '\0';
					
					char *tChunk_delimited;
					asprintf(&tChunk_delimited, "%s\r\n\r\n", tChunk);
					mChunk_t *aChunk = (mChunk_t *)deserializeMChunk(tChunk_delimited);
					addToDir(incomingD, key, aChunk, tKeys);
					lastBrkpt = pos + 1;
					free(tChunk);
					free(tChunk_delimited);
					break;
				}
			}
		}
	}
	
	//free(key); //DO NOT FREE KEY BECAUSE IT IS USED IN DICTIOANRY
	//return key;
}


char *serializeDir(dictionary_t *incomingD, queue_t *tKeys)
{
	//for each key in tKeys
	//dictionaryGet
	//serializeQ
	//append to overallS with @ as delimiter
	char *overallS = "", *temp = NULL;
	int set_once_flag = NOT_SET;
	
	unsigned int i;
	for(i=0; i<queue_size(tKeys); i++)
	{
		char *key = (char *)queue_at(tKeys, i);
		queue_t *currQ = (queue_t *)dictionary_get(incomingD, key);
		char *currQ_S = serializeQ(currQ, key, DONT_ADD_DATA, NOT_SET, NOT_SET);
		//must remove /r/n/r/n delimiter
		currQ_S[strlen(currQ_S) - ME_DELIM_SIZE] = '\0';
		
		if(set_once_flag == NOT_SET)
		{
			asprintf(&overallS, "%s", currQ_S);
			free(currQ_S);
			set_once_flag = IS_SET;
		}
		else
		{
			temp = overallS;
			asprintf(&overallS, "%s@%s", overallS, currQ_S);
			free(currQ_S);
			free(temp);
		}
	}
	
	//next we must perform some final touchups
	temp = overallS;
	asprintf(&overallS, "%s\r\n\r\n", overallS);
	free(temp);
	return overallS;
}


void deserializeDir(char *serializedDir, dictionary_t *incomingD, queue_t *tKeys)
{
	//parse based on @
	//deserializeQ
	
	//remove /r/n/r/n delimiter
	serializedDir[strlen(serializedDir) - ME_DELIM_SIZE] = '\0';
	
	//Next we parse
	int reached_end = NOT_SET;
	int pos, lastBrkpt = 0;
	
	while(reached_end == NOT_SET)
	{
		for(pos = lastBrkpt; pos < (int)strlen(serializedDir); pos++)
		{
			if(pos == (int)strlen(serializedDir) - 1)
			{
				reached_end = IS_SET;
				//This is the case for the last Queue in the Dir, or
				//if there is only one queue in the Dir
				
				char *tQ;
				asprintf(&tQ, "%s\r\n\r\n", serializedDir + lastBrkpt);
				char *key = extractKeyFromSerializedQ(tQ);
				deserializeQ(tQ, key, incomingD, tKeys);
				free(tQ);
				break;
			}
			else
			{
				if(serializedDir[pos] == '@')
				{
					char *tQ = (char *)malloc(pos - lastBrkpt + 1);
					strncpy(tQ, serializedDir + lastBrkpt, pos - lastBrkpt);
					tQ[pos - lastBrkpt] = '\0';
					
					char *tQ_delimited;
					asprintf(&tQ_delimited, "%s\r\n\r\n", tQ);
					char *key = extractKeyFromSerializedQ(tQ_delimited);
					deserializeQ(tQ_delimited, key, incomingD, tKeys);
					lastBrkpt = pos +1;
					free(tQ);
					free(tQ_delimited);
					break;
				}
			}
		}
	}
	return;
}









//===============Lucy's functions
int existsinML(queue_t *ML, mPayload_t *Payload)
{
	int result = 0;
	char *ip_addr = Payload->IP_address;
	int tPort = Payload->port;
	
	unsigned int i;
	for(i=0; i<queue_size(ML); i++)
	{
		mPayload_t *tempPayload = (mPayload_t *)queue_at(ML, i);
	    if((strcmp(ip_addr, tempPayload->IP_address) == 0) && (tPort==tempPayload->port))
		{
			result = 1;
			break;
	    }
	}
	return result;
}
