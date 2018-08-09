#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "reasonCode.h"
#include "rpc.h"

using namespace std;

int createSocket();

int bindAddress(int socketFd);

int socketListen(int socketFd);

int socketAccept(int socketFd);

int socketConnectTo(int socketFd, const char* serverAddress, int serverPort);

int recvMsgType(int localFd, MessageType &messageType);

/**********************REGISTER REQUEST**************************/
int sendRegReq(int localFd, string server_identifier, unsigned short port, string name, int argTypes[]);

int recvRegReq(int localFd, char* &serverID, unsigned short &port, char* &name, int* &argTypes);

int sendRegResSuccess(int localFd, ReasonCode code);

int recvRegReqSuccess(int localFd, ReasonCode &reasonCode);

int sendRegResFailure(int localFd, ReasonCode code);

int recvRegReqFailure(int localFd, ReasonCode &reasonCode);
/****************************************************************/

/**********************LOCATION REQUEST**************************/
int sendLocReq(int localFd, string name, int argTypes[]);

int recvLocReq(int localFd, char* &name, int* &argTypes);

int sendLocResSuccess(int localFd, string server_identifier, unsigned short port);

int recvLocReqSuccess(int localFd, char* &serverID, unsigned short &port);

int sendLocResFailure(int localFd, ReasonCode code);

int recvLocReqFailure(int localFd, ReasonCode &reasonCode);
/****************************************************************/

/**********************LOCATION REQUEST**************************/
int sendCacheReq(int localFd, string name, int argTypes[]);

int recvCacheReq(int localFd, char* &name, int* &argTypes);

int sendCacheNumber(int localFd, int num);

int recvCacheNumber(int localFd, int &num);

int sendCacheResSuccess(int localFd, string server_identifier, unsigned short port);

int recvCacheReqSuccess(int localFd, char* &serverID, unsigned short &port);

int sendCacheResFailure(int localFd, ReasonCode code);

int recvCacheReqFailure(int localFd, ReasonCode &reasonCode);
/****************************************************************/

/**********************EXECUTE REQUEST***************************/
int sendExeReq(int localFd, string name, int argTypes[], void**args);

int recvExeReq(int localFd, char* &name, int* &argTypes, void** &args);

int sendExeResSuccess(int localFd, string name, int argTypes[], void** args);

int recvExeReqSuccess(int localFd, char* &name, int* &argTypes, void** &args);

int sendExeResFailure(int localFd, ReasonCode code);

int recvExeReqFailure(int localFd, ReasonCode &reasonCode);
/****************************************************************/

/**********************TERMINATE REQUEST*************************/
int sendTermReq(int localFd);
/****************************************************************/

#endif