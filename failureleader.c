#include "failureleader.h"



void initHeartbeatSystem(queue_t *incomingML,char* myip, int myport,dictionary_t* mLocalDir,
                 queue_t* mKeys,pthread_mutex_t* mList_mutex,char* mdnsip, int mdnsp)
{
	myIP = myip;
        myPort = myport;
        myDic = mLocalDir;
        myKeys = mKeys;
        mUdpsock = createbindUDPSocket(myPort + 1);
        ml_mutex = mList_mutex;
        mDNSIP = mdnsip;
        mDNSport = mdnsp;
        pre = -1;
        suc = -1;
        
  
        findPredecessor(incomingML,myIP,myPort,1);
        findSuccessor(incomingML,myIP,myPort);

	if(pthread_create(&alive, NULL, livefunc, (void *)incomingML) != 0)
	{
		perror("pthread_create:alive\n");
	}
	
	if(pthread_create(&checking, NULL, checkfunc, (void *)incomingML) != 0)
	{
		perror("pthread_creat:checking\n");
	}
	
}

//find the predecessor in current ring from whom we will receive heartbeat
//return the index of predecessor or -1 if not available
//if detect is one , we update the timestamp for predecessor to detect failure
int findPredecessor(queue_t *mlp, char* ip, int port,int detect){
     if (queue_size(mlp) <= 1){
        //printf("queue isn't a ring\n");
        pre = -1;
        return -1;// it is not a ring
     }

     unsigned int i = 0;
     pthread_mutex_lock(ml_mutex);
     for(; i < queue_size(mlp); i++)
     {
		mPayload_t * temp = (mPayload_t *)queue_at(mlp,i);
		if(strcmp(temp->IP_address, ip) == 0 && temp->port == port)
		{
			if (i == 0)
                          pre =  queue_size(mlp) - 1;
                        else
                          pre = i - 1; 
                        printf("------------------my pre is %d and my location is %d\n",pre,i);
                        if (detect){
                           mPayload_t * predecessor = (mPayload_t *)queue_at(mlp,pre);
                           time(&predecessor->hb_timestamp);
                        }
                        break;  
		}
     }
     pthread_mutex_unlock(ml_mutex);
     if (i == queue_size(mlp))
         fprintf(stderr,"ERROR: couldn't find predecessor");
     return pre;
}

//find the successor in current ring to whom we will send heartbeat
//return the index of successor or -1 if not available
int findSuccessor(queue_t *mlp, char* ip, int port){
     if (queue_size(mlp) <= 1){
           //printf("queue isn't a ring\n");
           suc = -1;
           return -1;// it is not a ring
     }

     unsigned int i = 0;
     pthread_mutex_lock(ml_mutex);
     for(; i < queue_size(mlp); i++)
     {
		mPayload_t * temp = (mPayload_t *)queue_at(mlp,i);
		if(strcmp(temp->IP_address, ip) == 0 && temp->port == port)
		{
			if (i == queue_size(mlp) - 1)
                          suc = 0 ;
                        else
                          suc = i + 1; 
                        printf("------------------my suc is %d and my location is %d\n",suc,i);
                        break;  
		}
     }
     pthread_mutex_unlock(ml_mutex);
     if (i == queue_size(mlp))
         fprintf(stderr,"ERROR: couldn't find successor");
     return suc;
}

int createUDPSocket(){//non-bind version
    int s;
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
        perror("socket:");
		exit(1);
	}
    return s;
}


int createbindUDPSocket(int udpport)
{
	//this is bind version
	struct sockaddr_in si_me;
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(udpport);
	
	struct hostent *localHost = gethostbyname(myIP);
	si_me.sin_addr = *((struct in_addr *)localHost->h_addr);
	
	
	int s = createUDPSocket();
	
	if(bind(s,(struct sockaddr*)&si_me,sizeof(si_me)) < 0)
	{
		perror("socket:");
		exit(1);
	}
	
	return s;
}




void updatetimestamp(char* buf,queue_t *mlp)
{
    
    mPayload_t* temppayload = deserializePayload(buf);
    int temport = temppayload->port;
    int tempseq = temppayload->seq_num;
    //printf("received heartbeat from %s:%d with sequence %d\n",temppayload->IP_address,temport,tempseq);
    
    if (pre != -1){
      pthread_mutex_lock(ml_mutex);
      mPayload_t * temp = (mPayload_t *)queue_at(mlp,pre);
      pthread_mutex_unlock(ml_mutex);
      if(strcmp(temp->IP_address,temppayload->IP_address) == 0 && temp->port == temport)
      {
         if(temp->seq_num < tempseq)
         {
	   temp->seq_num = tempseq;
	   time(&temp->hb_timestamp);
	   //printf("update heartbeat entry for %s:%d\n",temppayload->IP_address,temport);

         }
      }
      else{
         fprintf(stderr,"I received stranger's heartbeat-->==%s:%d== ==%s:%d==\n", temp->IP_address, temp->port, temppayload->IP_address, temppayload->port);
      }
    }
    destroyPayload(temppayload);
}


void sendUDPmessage(char *ip, int port, int sockfd, mPayload_t * payload, int payloadaction)
{
	if(sockfd == -1)
		sockfd = createUDPSocket();
	struct sockaddr_in si_temp;
	si_temp.sin_family = AF_INET;
	si_temp.sin_port = htons(port);
	if(inet_aton(ip,&si_temp.sin_addr) != 1)
		fprintf(stderr,"inet_aton at sendUDPmessage for ip %s is wrong:",ip);
	char *msg = serializePayload(payload,payloadaction);
	
	if(sendto(sockfd,msg,strlen(msg) + 1,0, (const struct sockaddr *)&si_temp,sizeof(si_temp)) == -1)
            perror("sendto:");
	//printf("msg to send: %s with action %d\n", msg, payloadaction);
	free(msg);
}


void *livefunc(void *arg)
{     
	queue_t *mlp = (queue_t *)arg;
	int seq = 0;//initial sequence number;
	while(1)
	{
		seq ++;

		mPayload_t* heartbeatpack = generatePayload(myIP,myPort);
		heartbeatpack->seq_num = seq;
               
                if (suc != -1){
                  pthread_mutex_lock(ml_mutex);
                  mPayload_t * temp = (mPayload_t *)queue_at(mlp,suc);
                  pthread_mutex_unlock(ml_mutex);
                
                  sendUDPmessage(temp->IP_address,temp->port + 1,temp->udp_socket,heartbeatpack,HEARTBEAT);
		  //printf("send heartbeat to %s:%d with seq: %d\n",temp->IP_address,temp->port + 1, seq);
                }
		free(heartbeatpack);
		sleep(HEARTBEATTIME);
	}
	pthread_exit(NULL);
}



void *checkfunc(void *arg)
{    
	queue_t *mlp = (queue_t *)arg;
	while(1)
	{

		time_t currenttime;
		if (pre != -1)
		{
			pthread_mutex_lock(ml_mutex);
			mPayload_t * temp = (mPayload_t *)queue_at(mlp,pre);
			pthread_mutex_unlock(ml_mutex);
			time(&currenttime);
			if(currenttime - temp->hb_timestamp > FAILURETIME)
	             failorleave(FAILURE,temp,1,mlp);			
		}

		sleep(CHECKINGTIME);
                //printMembershipList(mlp, NULL);
	}
	pthread_exit(NULL);
}


int failorleave(int flag, mPayload_t *temp,int founder,queue_t *mlp)
{  
   int isleader = 0;
   if(existsinML(mlp,temp)){
            
     pthread_mutex_lock(ml_mutex);
     mPayload_t * leader = (mPayload_t *)queue_at(mlp,0);
     pthread_mutex_unlock(ml_mutex);
     suc = -1; 
     pre = -1;

     //before we change the queue we need to find out the leave/failed machine's pre and suc to send its replicas to        
     int despre = findPredecessor(mlp, temp->IP_address, temp->port,0);
     int dessuc = findSuccessor(mlp, temp->IP_address, temp->port);
     if (despre == -1 || dessuc == -1){
         fprintf(stderr,"ERROR: no pre or no suc");
     }
     pthread_mutex_lock(ml_mutex);
     mPayload_t * desprepayload = (mPayload_t *)queue_at(mlp,despre);
     mPayload_t * dessucpayload = (mPayload_t *)queue_at(mlp,dessuc);
     pthread_mutex_unlock(ml_mutex);


     int desprepre = findPredecessor(mlp, desprepayload->IP_address,desprepayload->port,0);
     int dessucsuc = findSuccessor(mlp, dessucpayload->IP_address,dessucpayload->port);
     if (desprepre == -1 || dessucsuc == -1){
         fprintf(stderr,"ERROR: no pre or no suc");
     }
     pthread_mutex_lock(ml_mutex);
     mPayload_t * despreprepayload = (mPayload_t *)queue_at(mlp,desprepre);
     mPayload_t * dessucsucpayload = (mPayload_t *)queue_at(mlp,dessucsuc);
     pthread_mutex_unlock(ml_mutex);
     
 
     if(strcmp(temp->IP_address,leader->IP_address) == 0 && temp->port == leader->port)
         isleader = 1;     
     if(flag == FAILURE)
     {
       mprintf("[%s:%d] has FAILED\n",temp->IP_address,temp->port);
     }
     else{
       mprintf("[%s:%d] has LEFT\n",temp->IP_address,temp->port);
     }
     pthread_mutex_lock(ml_mutex);
     removeFromML(mlp, temp);
     pthread_mutex_unlock(ml_mutex);
     findPredecessor(mlp,myIP,myPort,1);
     findSuccessor(mlp,myIP,myPort);

     if (founder){//only founder should multicast the failure
         mPayload_t* multicastpack = generatePayload(temp->IP_address,temp->port);
         multicastMsg(mlp, multicastpack, flag, -1);
         //destroyPayload(temp);//shouldn't destroy here
         destroyPayload(multicastpack);
     }

     if(isleader){
         printf("--------------leader died\n");
         leaderhelper(mlp);        
     }

     if (queue_size(mlp) > 1)//we only need to handle re-replicate when there are more than 1 machine
        handlereplicas(temp->IP_address,temp->port,desprepayload->IP_address,desprepayload->port,
             dessucpayload->IP_address,dessucpayload->port,despreprepayload->IP_address,despreprepayload->port,dessucsucpayload->IP_address,dessucsucpayload->port);
     else//all copies on me are original
        alloriginal(temp->IP_address,temp->port);
     //anything related to mlp needs mutex
     destroyPayload(temp);
   }
   return isleader;  
	
}

void leaderhelper(queue_t *mlp){
   pthread_mutex_lock(ml_mutex);
   mPayload_t * leader = (mPayload_t *)queue_at(mlp,0);
   pthread_mutex_unlock(ml_mutex);

   if(strcmp(leader->IP_address, myIP) == 0 && leader->port == myPort){
        printf("-----------I'm leader now\n");
        int DNSsock = createSocketClient(mDNSIP, mDNSport);
        mPayload_t* leaderpack = generatePayload(myIP,myPort);//send out my local IP and port
        sendPayload(leaderpack, DNSsock, FAILURE); 
   }
}

// this function is called when 1 of 2 machines failed
void alloriginal(char* ip, int port){
    unsigned int i;
    unsigned int j;
    for(i = 0; i < queue_size(myKeys); i++){
       char *key = queue_at(myKeys, i);
       queue_t *tempQ = (queue_t *)dictionary_get(myDic, key);
       for (j = 0;j < queue_size(tempQ); j++){
           mChunk_t *tempC = (mChunk_t *)queue_at(tempQ, j);
           if(strcmp(tempC->ip_addr,ip) == 0 && tempC->port == port){
                 queue_remove_at(tempQ, j);
                 j --;
           }
           else{//it is my chunk
               tempC->chunkType = ORIGINAL; 
           }
       }
    }


}


//everybody needs to update its own dictionary and send out the local 
//replica if the other is on the failed or left machine
void handlereplicas(char* ip, int port,char* preip, int preport, char* sucip,int sucport,char* prepreip, int prepreport, char* sucsucip,int sucsucport){//if there is one left, do nothing
    unsigned int i;
    unsigned int j;
    unsigned int k;
    fprintf(stderr,"calling handlereplicas\n");
    for(i = 0; i < queue_size(myKeys); i++){
       char *key = queue_at(myKeys, i);
       queue_t *tempQ = (queue_t *)dictionary_get(myDic, key);
       for (j = 0;j < queue_size(tempQ); j++){
           mChunk_t *tempC = (mChunk_t *)queue_at(tempQ, j);
          // printf("failed guy is %s and %d and the current queue is %s and %d\n",ip,port,tempC->ip_addr,tempC->port);
           if(strcmp(tempC->ip_addr,ip) == 0 && tempC->port == port){//now need to send the replica if I have it
                 printf("%s with chunk id %d and chunk type %d is missing\n",key,tempC->chunkID,tempC->chunkType);
                 char* desip;
                 int desport;
                 if (tempC->chunkType == ORIGINAL){
                      desip = preip;
                      desport = preport;
                 }
                 else{
                      desip = sucip;
                      desport = sucport;
                 }
                 //this is a special case when only two servers have all chunks of a particular file and one of them failed and there
                 //are still more than 1 machine in the system
                 for(k = 0;k < queue_size(tempQ); k++){ 
                      mChunk_t *tempkC = (mChunk_t *)queue_at(tempQ, k);
                      if(k != j && tempkC->chunkID == tempC->chunkID && 
                          strcmp(tempkC->ip_addr,desip) == 0 && tempkC->port == desport){
                          if (tempC->chunkType == ORIGINAL){
                                   desip = prepreip;
                                   desport = prepreport;
                          }
                          else{
                              desip = sucsucip;
                              desport = sucsucport;
                          }
                      }
                 }
                 printf("the destination ip is %s and port is %d\n",desip,desport);
            
                 //try to find do I have this chunk
                 for(k = 0;k < queue_size(tempQ); k++){ 
                      mChunk_t *tempkC = (mChunk_t *)queue_at(tempQ, k);
                      if(k != j && tempkC->chunkID == tempC->chunkID && 
                          strcmp(tempkC->ip_addr,myIP) == 0 && tempkC->port == myPort){
                             //we found another copy and it is on me so I just send it to destination
                             printf("%s with chunk id %d is on me\n", key, tempkC->chunkID);
                             int dessock = createSocketClient(desip, desport);
                             char *filename;
	                     asprintf(&filename, "[%s:%d]%s_%d", tempkC->ip_addr, tempkC->port, key, tempkC->chunkID);
                             FILE *mFile = fopen(filename, "rb");
	                     char* chunkfile = (char *)malloc( (tempkC->chunkSize * sizeof(char)) + 1);
						  printf("Tying to read from file: %s\n", filename);
	                     fread(chunkfile, 1, tempkC->chunkSize, mFile);
                             //next we must null terminate because fread does not do so.
	                     chunkfile[tempkC->chunkSize] = '\0';
                             fclose(mFile);
                             free(filename);                            
                             asprintf(&filename, "[%s:%d]%s_%d", desip, desport, key, tempkC->chunkID);
                             mPayload_t* chunkpack = generatePayload(filename,myPort);
                             chunkpack->timestamp = chunkfile;
                             sendPayload(chunkpack, dessock, FAILURE_SHIFT);
                             free(filename);
                             destroyPayload(chunkpack);                          
                      }

                 }
                 //update the local dictionary chunck
                 asprintf(&tempC->ip_addr, "%s",desip);
		 tempC->port = desport;
            }

       }

    }

}


int receiveUdpmsg(queue_t *mlp)
{
        printf("listen for heartbeats\n");
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	//char buf[50]; //50 is too small
	char buf[512];
	while(1)
	{
		if(recvfrom(mUdpsock,buf,sizeof(buf),0,(struct sockaddr *)&si_other,(socklen_t *)&slen) == -1)
			fprintf(stderr,"error from recvfrom\n");
		//printf("buf is %s and action is %d S\n",buf,getPayloadAction(buf));

		int payload_action = getPayloadAction(buf);
		if(payload_action == HEARTBEAT)
		{
			updatetimestamp(buf,mlp);
		}
		else
		{
			printf("received packet that isn't heartbeat from udp port: %d\n", payload_action);
		}
	}
	return 0;
}

void destoryHeartbeatSystem()
{
	pthread_kill(alive, SIGKILL);
	pthread_kill(checking, SIGKILL);
}
