CC = gcc
INC = -I.
FLAGS = -g -W -Wall
LIBS = -lpthread

all: mServer mClient mDNS mFileGen

mServer: queue.o libdictionary.o payload.o mSocketLib.o mLoggingLib.o mFileLib.o failureleader.o mServer.c mServer.h
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)
	
mClient: queue.o libdictionary.o payload.o mSocketLib.o mLoggingLib.o mFileLib.o mClient.c mClient.h
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)

mDNS: queue.o libdictionary.o payload.o mSocketLib.o mLoggingLib.o mFileLib.o mDNS.c mDNS.h
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)
    	
queue.o: queue.c queue.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@
	
libdictionary.o: libdictionary.c libdictionary.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)
	
payload.o: payload.c payload.h  
	$(CC) -c $(FLAGS) $(INC) $< -o $@

mSocketLib.o: mSocketLib.c mSocketLib.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@

mLoggingLib.o: mLoggingLib.c mLoggingLib.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@

mFileLib.o: mFileLib.c mFileLib.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@

failureleader.o: failureleader.c failureleader.h 
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)
	
mFileGen: mFileGen.c
	$(CC) $(FLAGS) $^ -o $@
clean:
	rm -rf *.o *.d mServer mDNS *.log *~
