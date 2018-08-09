#include "socket.h"

using namespace std;

int createSocket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

int bindAddress(int socketFd) {
    struct sockaddr_in address;
    memset((struct sockaddr_in *)&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0;

    if (bind(socketFd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        return SOCKET_LOCAL_BIND_FALIURE;
    }
}

int socketListen(int socketFd) {
    if (listen(socketFd, 5) < 0) {
        return SOCKET_LOCAL_LISTEN_FALIURE;
    }
}

int socketAccept(int socketFd) {
    struct sockaddr_in address;
    socklen_t addressSize = sizeof(address);

    int newSocketFd = accept(socketFd, (struct sockaddr *) &address, &addressSize);
    if (newSocketFd < 0) {
        return SOCKET_ACCEPT_CLIENT_FAILURE;
    }

    return newSocketFd;
}

int socketConnectTo(int socketFd, const char * serverAddress, int serverPort) {
    struct hostent *server;
    struct sockaddr_in sa;
    
    server = gethostbyname(serverAddress);
    if (server == NULL) {
       return SOCKET_UNKNOWN_HOST;
    }

    memset((char *) &sa, 0,sizeof(sa));
    sa.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&sa.sin_addr.s_addr,
         server->h_length);
    sa.sin_port = htons(serverPort);

    if (connect(socketFd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        return SOCKET_CONNECTION_FALIURE;
    }

    return 0;
}

int recvMsgType(int localFd, MessageType &messageType) {
    int temp, type;
    if (temp = recv(localFd, &type, 4, 0) <= 0) {
        // cout << "Msg Type Receiving Fails" << endl;
        // sometimes it's intendend to fail
        return temp;
    }

    messageType = (MessageType)type;
    return 0;
}

/**********************REGISTER REQUEST**************************/
int sendRegReq(int localFd, string server_identifier, unsigned short port, string name, int argTypes[]) {
    unsigned int serverIDSize = server_identifier.length() + 1;
    unsigned int portSize = 2;

    unsigned int nameSize = name.size() + 1;

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    unsigned int argTypesSize = argTypesLength * 4;

    int temp;
    int type = REGISTER;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg REGISTER Sending Fails" << endl;
        return temp;
    }
    // send server id length
    // cout << serverIDSize << endl;
    if (temp = send(localFd, &serverIDSize, 4, 0) < 0) {
        cout << "Msg Server ID length Sending Fails" << endl;
        return temp;
    } 
    // send server id
    // cout << server_identifier.c_str() << endl;
    if (temp = send(localFd, server_identifier.c_str(), serverIDSize, 0) < 0) {
        cout << "Msg Server ID Sending Fails" << endl;
        return temp;
    }
    // send port length
    if (temp = send(localFd, &portSize, 4, 0) < 0) {
        cout << "Msg port length Sending Fails" << endl;
        return temp;
    }
    // send port
    // cout << port << endl;
    if (temp = send(localFd, &port, portSize, 0) < 0) {
        cout << "Msg port Sending Fails" << endl;
        return temp;
    } 
    // send program name size
    if (temp = send(localFd, &nameSize, 4, 0) < 0) {
        cout << "Msg name size Sending Fails" << endl;
        return temp;
    } 
    // send program name
    // cout << name << endl;
    if (temp = send(localFd, name.c_str(), nameSize, 0) < 0) {
        cout << "Msg name Sending Fails" << endl;
        return temp;
    } 
    // send argTypes size
    if (temp = send(localFd, &argTypesSize, 4, 0) < 0) {
        cout << "Msg argTypes Sending Fails" << endl;
        return temp;
    } 
    // send argTypes 
    for (int i = 0; i < argTypesLength; i++) {
        // cout << argTypes[i] << endl;
        if (temp = send(localFd, &argTypes[i], 4, 0) < 0) {
            cout << "Msg argTypes Sending Fails" << endl;
            return temp;
        }
    }

    return 0;
}

int recvRegReq(int localFd, char* &serverID, unsigned short &port, char* &name, int* &argTypes) {
    int temp;
    int serverIDSize = 0, portSize = 2;
    int nameSize = 0, argTypesSize = 0;

    // receive server id size
    if (temp = recv(localFd, &serverIDSize, 4, 0) <= 0) {
        cout << "Msg ServerID Size Receive Fails" << endl;
        return temp;
    }
    // cout << serverIDSize << endl;
    // receive server id
    serverID = new char[serverIDSize];
    if (temp = recv(localFd, serverID, serverIDSize, 0) <= 0) {
        cout << "Msg ServerID Receive Fails" << endl;
        return temp;
    }
    // cout << serverID << endl;
    // receive port size
    if (temp = recv(localFd, &portSize, 4, 0) <= 0) {
        cout << "Msg Port Size Receive Fails" << endl;
        return temp;
    }
    // receive port
    unsigned short temp2 = 0;
    if (temp = recv(localFd, &temp2, portSize, 0) <= 0) {
        cout << "Msg Port Receive Fails" << endl;
        return temp;
    } 
    // cout << temp2 << endl;
    port = temp2;
    // receive name size
    if (temp = recv(localFd, &nameSize, 4, 0) <= 0) {
        cout << "Msg Name Size Receive Fails" << endl;
        return temp;
    }
    // receive name
    name = new char[nameSize];
    if (temp = recv(localFd, name, nameSize, 0) <= 0) {
        cout << "Msg Name Receive Fails" << endl;
        return temp;
    }
    // cout << name << endl;
    // receive argTypes size
    if (temp = recv(localFd, &argTypesSize, 4, 0) <= 0) {
        cout << "Msg ArgTypes Size Receive Fails" << endl;
        return temp;
    }
    int argTypesLength = argTypesSize/sizeof(int);
    // receive argTypes
    int argType = 0, index = 0;
    // allocate memory for argTypes
    argTypes = new int[argTypesLength];
    while (argTypesSize > 0) {
        if (temp = recv(localFd, &argType, 4, 0) <= 0) {
            cout << "Msg ArgType Receive Fails" << endl;
            return temp;
        }
        argTypesSize -= temp;
        argTypes[index] = argType;
        index += 1;
        if (argType == 0) {
            break; // terminator
        } else {
            argType = 0; // reset arg type
        }
    }
    // unsigned int leng = 0;
	// while (argTypes[leng++]);
    // cout << leng << endl;
	// for (int i = 0; i < leng; i++) {
	// 	cout << argTypes[i] << endl;
	// }
    return 0;
}

int sendRegResSuccess(int localFd, ReasonCode code) {
    int temp;
    int type = REGISTER_SUCCESS;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg EXECUTE_SUCCESS Sending Fails" << endl;
        return temp;
    }
    
    if (temp = send(localFd, &code, 4, 0) < 0) {
        cout << "Msg Success Code Sending Fails" << endl;
        return temp;
    }

    return 0;
}

int recvRegReqSuccess(int localFd, ReasonCode &reasonCode) {
    int temp, temp2;
    if (temp = recv(localFd, &temp2, 4, 0) <= 0) {
        cout << "Msg Register Request Success ReasonCode Receive Fails" << endl;
        return temp;
    } 
    reasonCode = (ReasonCode)temp2;
    return 0;    
}

int sendRegResFailure(int localFd, ReasonCode code) {
    int temp;
    int type = REGISTER_FAILURE;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg RESGISTER_FAILUTER Sending Fails" << endl;
        return temp;
    }
    if (temp = send(localFd, &code, 4, 0) < 0) {
        cout << "Msg Error Code Sending Fails" << endl;
        return temp;
    } 
    return 0;
}

int recvRegReqFailure(int localFd, ReasonCode &reasonCode) {
    int temp, temp2;
    if (temp = recv(localFd, &temp2, 4, 0) <= 0) {
        cout << "Msg Register Request Failure ReasonCode Receive Fails" << endl;
        return temp;
    } 
    reasonCode = (ReasonCode)temp2;
    return 0;
}
/****************************************************************/

/**********************LOCATION REQUEST**************************/
int sendLocReq(int localFd, string name, int argTypes[]) {
    unsigned int nameSize = name.size() + 1;

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    unsigned int argTypesSize = argTypesLength * 4;
    
    int temp;
    int type = LOC_REQUEST;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg LOC_REQUEST Sending Fails" << endl;
        return temp;
    } 
    // send program name size
    if (temp = send(localFd, &nameSize, 4, 0) < 0) {
        cout << "Msg name size Sending Fails" << endl;
        return temp;
    } 
    // send program name
    // cout << name << endl;
    if (temp = send(localFd, name.c_str(), nameSize, 0) < 0) {
        cout << "Msg name Sending Fails" << endl;
        return temp;
    } 
    // send argTypes size
    if (temp = send(localFd, &argTypesSize, 4, 0) < 0) {
        cout << "Msg argTypes Sending Fails" << endl;
        return temp;
    } 
    // send argTypes 
    for (int i = 0; i < argTypesLength; i++) {
        // cout << argTypes[i] << endl;
        if (temp = send(localFd, &argTypes[i], 4, 0) < 0) {
            cout << "Msg argTypes Sending Fails" << endl;
            return temp;
        }
    }

    return 0;
}

int recvLocReq(int localFd, char* &name, int* &argTypes) {
    int temp;
    int nameSize = 0, argTypesSize = 0;

    // receive name size
    if (temp = recv(localFd, &nameSize, 4, 0) <= 0) {
        cout << "Msg Name Size Receive Fails" << endl;
        return temp;
    }
    // receive name
    name = new char[nameSize];
    if (temp = recv(localFd, name, nameSize, 0) <= 0) {
        cout << "Msg Name Receive Fails" << endl;
        return temp;
    }
    // cout << name << endl;
    // receive argTypes size
    if (temp = recv(localFd, &argTypesSize, 4, 0) <= 0) {
        cout << "Msg ArgTypes Size Receive Fails" << endl;
        return temp;
    }
    int argTypesLength = argTypesSize/sizeof(int);
    // receive argTypes
    int argType = 0, index = 0;
    // allocate memory for argTypes
    argTypes = new int[argTypesLength];
    while (argTypesSize > 0) {
        if (temp = recv(localFd, &argType, 4, 0) <= 0) {
            cout << "Msg ArgType Receive Fails" << endl;
            return temp;
        }
        argTypesSize -= temp;
        argTypes[index] = argType;
        index += 1;
        if (argType == 0) {
            break; // terminator
        } else {
            argType = 0; // reset arg type
        }
    }
    // unsigned int leng = 0;
	// while (argTypes[leng++]);
    // cout << leng << endl;
	// for (int i = 0; i < leng; i++) {
	// 	cout << argTypes[i] << endl;
	// }
    
    return 0;
}

int sendLocResSuccess(int localFd, string server_identifier, unsigned short port) {
    unsigned int serverIDSize = server_identifier.length() + 1;
    unsigned int portSize = 2;

    int temp;
    int type = LOC_SUCCESS;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg LOC_SUCCESS Sending Fails" << endl;
        return temp;
    } 
    // send server id length
    // cout << serverIDSize << endl;
    if (temp = send(localFd, &serverIDSize, 4, 0) < 0) {
        cout << "Msg Server ID length Sending Fails" << endl;
        return temp;
    } 
    // send server id
    // cout << server_identifier.c_str() << endl;
    if (temp = send(localFd, server_identifier.c_str(), serverIDSize, 0) < 0) {
        cout << "Msg Server ID Sending Fails" << endl;
        return temp;
    }
    // send port length
    if (temp = send(localFd, &portSize, 4, 0) < 0) {
        cout << "Msg port length Sending Fails" << endl;
        return temp;
    }
    // send port
    // cout << port << endl;
    if (temp = send(localFd, &port, portSize, 0) < 0) {
        cout << "Msg port Sending Fails" << endl;
        return temp;
    }
    
    return 0;
}

int recvLocReqSuccess(int localFd, char* &serverID, unsigned short &port) {
    int temp;
    int serverIDSize = 0, portSize = 2;

    // receive server id size
    if (temp = recv(localFd, &serverIDSize, 4, 0) <= 0) {
        cout << "Msg ServerID Size Receive Fails" << endl;
        return temp;
    }
    // cout << serverIDSize << endl;
    // receive server id
    serverID = new char[serverIDSize];
    if (temp = recv(localFd, serverID, serverIDSize, 0) <= 0) {
        cout << "Msg ServerID Receive Fails" << endl;
        return temp;
    }
    // cout << serverID << endl;
    // receive port size
    if (temp = recv(localFd, &portSize, 4, 0) <= 0) {
        cout << "Msg Port Size Receive Fails" << endl;
        return temp;
    }
    // receive port
    unsigned short temp2 = 0;
    if (temp = recv(localFd, &temp2, portSize, 0) <= 0) {
        cout << "Msg Port Receive Fails" << endl;
        return temp;
    } 
    // cout << temp2 << endl;
    port = temp2;

    return 0;
}

int sendLocResFailure(int localFd, ReasonCode code) {
    int temp;
    int type = LOC_FAILURE;
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg LOC_FAILURE Sending Fails" << endl;
        return temp;
    } 
    if (temp = send(localFd, &code, 4, 0) < 0) {
        cout << "Msg Error Code Sending Fails" << endl;
        return temp;
    } 
    return 0;
}

int recvLocReqFailure(int localFd, ReasonCode &reasonCode) {
    int temp, temp2;
    if (temp = recv(localFd, &temp2, 4, 0) <= 0) {
        cout << "Msg LocReq ReasonCode Receive Fails" << endl;
        return temp;
    } 

    reasonCode = (ReasonCode)temp2;
    return 0;
}
/****************************************************************/

/**********************CACHE REQUEST**************************/
int sendCacheReq(int localFd, string name, int argTypes[]) {
    unsigned int nameSize = name.size() + 1;

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    unsigned int argTypesSize = argTypesLength * 4;
    
    int temp;
    int type = CACHE_REQUEST;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg CACHE_REQUEST Sending Fails" << endl;
        return temp;
    } 
    // send program name size
    if (temp = send(localFd, &nameSize, 4, 0) < 0) {
        cout << "Msg name size Sending Fails" << endl;
        return temp;
    } 
    // send program name
    // cout << name << endl;
    if (temp = send(localFd, name.c_str(), nameSize, 0) < 0) {
        cout << "Msg name Sending Fails" << endl;
        return temp;
    } 
    // send argTypes size
    if (temp = send(localFd, &argTypesSize, 4, 0) < 0) {
        cout << "Msg argTypes Sending Fails" << endl;
        return temp;
    } 
    // send argTypes 
    for (int i = 0; i < argTypesLength; i++) {
        // cout << argTypes[i] << endl;
        if (temp = send(localFd, &argTypes[i], 4, 0) < 0) {
            cout << "Msg argTypes Sending Fails" << endl;
            return temp;
        }
    }

    return 0;
}

int recvCacheReq(int localFd, char* &name, int* &argTypes) {
    int temp;
    int nameSize = 0, argTypesSize = 0;

    // receive name size
    if (temp = recv(localFd, &nameSize, 4, 0) <= 0) {
        cout << "Msg Name Size Receive Fails" << endl;
        return temp;
    }
    // receive name
    name = new char[nameSize];
    if (temp = recv(localFd, name, nameSize, 0) <= 0) {
        cout << "Msg Name Receive Fails" << endl;
        return temp;
    }
    // cout << name << endl;
    // receive argTypes size
    if (temp = recv(localFd, &argTypesSize, 4, 0) <= 0) {
        cout << "Msg ArgTypes Size Receive Fails" << endl;
        return temp;
    }
    int argTypesLength = argTypesSize/sizeof(int);
    // receive argTypes
    int argType = 0, index = 0;
    // allocate memory for argTypes
    argTypes = new int[argTypesLength];
    while (argTypesSize > 0) {
        if (temp = recv(localFd, &argType, 4, 0) <= 0) {
            cout << "Msg ArgType Receive Fails" << endl;
            return temp;
        }
        argTypesSize -= temp;
        argTypes[index] = argType;
        index += 1;
        if (argType == 0) {
            break; // terminator
        } else {
            argType = 0; // reset arg type
        }
    }
    // unsigned int leng = 0;
	// while (argTypes[leng++]);
    // cout << leng << endl;
	// for (int i = 0; i < leng; i++) {
	// 	cout << argTypes[i] << endl;
	// }
    
    return 0;
}

int sendCacheNumber(int localFd, int num) {
    int temp;
    if (temp = send(localFd, &num, 4, 0) < 0) {
        cout << "Msg Cache Number Send Fails" << endl;
        return temp;
    }

    return 0;
}

int recvCacheNumber(int localFd, int &num) {
    int temp;
    if (temp = recv(localFd, &num, 4, 0) <= 0) {
        cout << "Msg Cache Number Receive Fails" << endl;
        return temp;
    } 

    return 0;
}

int sendCacheResSuccess(int localFd, string server_identifier, unsigned short port) {
    return sendLocResSuccess(localFd, server_identifier, port);
}

int recvCacheReqSuccess(int localFd, char* &serverID, unsigned short &port) {
    return recvLocReqSuccess(localFd, serverID, port);
}

int sendCacheResFailure(int localFd, ReasonCode code) {
    return sendLocResFailure(localFd, code);
}

int recvCacheReqFailure(int localFd, ReasonCode &reasonCode) {
    return recvLocReqFailure(localFd, reasonCode);
}
/****************************************************************/

/**********************EXECUTE REQUEST***************************/
int sendExeReq(int localFd, string name, int argTypes[], void**args) {
    unsigned int nameSize = name.size() + 1;

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    unsigned int argTypesSize = argTypesLength * 4;

    int temp;
    int type = EXECUTE;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg LOC_REQUEST Sending Fails" << endl;
        return temp;
    } 
    // send program name size
    if (temp = send(localFd, &nameSize, 4, 0) < 0) {
        cout << "Msg name size Sending Fails" << endl;
        return temp;
    } 
    // send program name
    // cout << name << endl;
    if (temp = send(localFd, name.c_str(), nameSize, 0) < 0) {
        cout << "Msg name Sending Fails" << endl;
        return temp;
    } 
    // send argTypes size
    if (temp = send(localFd, &argTypesSize, 4, 0) < 0) {
        cout << "Msg argTypes Sending Fails" << endl;
        return temp;
    } 
    // send argTypes 
    for (int i = 0; i < argTypesLength; i++) {
        // cout << argTypes[i] << endl;
        if (temp = send(localFd, &argTypes[i], 4, 0) < 0) {
            cout << "Msg argTypes Sending Fails" << endl;
            return temp;
        }
    }

    // send args
    for (unsigned int i = 0; i < argTypesLength - 1; i++) {
        int argType = argTypes[i];

        // get length of this argument
        int argLength = (int)(argType & 65535);
        argLength = argLength? argLength: 1;
        // get type of this argument
        int type = ((argType >> 16) & 255);
        
        void* arg = args[i];

        // cout << "TYPE" << endl;
        // cout << type << endl;

        switch(type) {
            case ARG_CHAR: {
                char* chars = (char *)arg;
                for (int j = 0; j < argLength; j++) {
                    char c = *(chars + j);
                    if (temp = send(localFd, &c, 1, 0) < 0) {
                        cout << "Msg char array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND CHAR" << endl;
                    // cout << c << endl;
                }
                
            }
            break;
            case ARG_SHORT: {
                short* shortInts = (short*)arg;
                for (int j = 0; j < argLength; j++) {
                    short si = *(shortInts + j);
                    if (temp = send(localFd, &si, 2, 0) < 0) {
                        cout << "Msg short array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND SHORT" << endl;
                    // cout << si << endl;
                }
            }
            break;
            case ARG_INT: {
                // cout << "SEND INT ARG" << endl;
                int* Ints = (int*)arg;
                for (int j = 0; j < argLength; j++) {
                    int in = *(Ints + j);

                    // cout << "SEND ARG" << endl;
                    // cout << in << endl;

                    if (temp = send(localFd, &in, 4, 0) < 0) {
                        cout << "Msg int array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND SHORT" << endl;
                    // cout << in << endl;
                }
            }
            break;
            case ARG_LONG: {
                long* longInts = (long*)arg;
                for (int j = 0; j < argLength; j++) {
                    long li = *(longInts + j);
                    if (temp = send(localFd, &li, 4, 0) < 0) {
                        cout << "Msg long array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND LONG" << endl;
                    // cout << li << endl;
                }
            }
            break;
            case ARG_DOUBLE: {
                double* doubleInts = (double*)arg;
                for (int j = 0; j < argLength; j++) {
                    double d = *(doubleInts + j);
                    if (temp = send(localFd, &d, 8, 0) < 0) {
                        cout << "Msg double array Sending Fails" << endl;
                        return temp;
                    }
                }
            }
            break;
            case ARG_FLOAT: {
                float* floats = (float*)arg;
                for (int j = 0; j < argLength; j++) {
                    float f = *(floats + j);
                    if (temp = send(localFd, &f, 4, 0) < 0) {
                        cout << "Msg float array Sending Fails" << endl;
                        return temp;
                    }
                } 
            }
            break;
            default:
            break;
        }
    }

    return 0;
}

int recvExeReq(int localFd, char* &name, int* &argTypes, void** &args) {
    int temp;
    int nameSize = 0, argTypesSize = 0, argsSize = 0;

    // receive name size
    if (temp = recv(localFd, &nameSize, 4, 0) <= 0) {
        cout << "Msg Name Size Receive Fails" << endl;
        return temp;
    }
    // receive name
    name = new char[nameSize];
    if (temp = recv(localFd, name, nameSize, 0) <= 0) {
        cout << "Msg Name Receive Fails" << endl;
        return temp;
    }
    // cout << name << endl;
    // receive argTypes size
    if (temp = recv(localFd, &argTypesSize, 4, 0) <= 0) {
        cout << "Msg ArgTypes Size Receive Fails" << endl;
        return temp;
    }
    // receive argTypes
    int argTypesLength = argTypesSize/sizeof(int);
    int argType = 0, index = 0;
    // allocate memory for argTypes
    argTypes = new int[argTypesLength];
    while (argTypesSize > 0) {
        if (temp = recv(localFd, &argType, 4, 0) <= 0) {
            cout << "Msg ArgType Receive Fails" << endl;
            return temp;
        }
        argTypesSize -= temp;
        argTypes[index] = argType;
        index += 1;
        if (argType == 0) {
            break; // terminator
        } else {
            argType = 0; // reset arg type
        }
    }
    // unsigned int leng = 0;
	// while (argTypes[leng++]);
    // cout << leng << endl;
	// for (int i = 0; i < leng; i++) {
	// 	cout << argTypes[i] << endl;
	// }

    // receive args
    for (int i = 0; i < argTypesLength - 1; i++) {
        int argTypeValue = argTypes[i];
        
        int argLength = (int)(argTypeValue & 65535);
        // argLength = argLength? argLength: 1;

        int type = ((argTypeValue >> 16) & 255);

        // cout << "TYPE" << endl;
        // cout << type << endl;

        switch(type) {
            case ARG_CHAR: {
                if (argLength == 0) {
                    // scalar
                    char c;
                    if (temp = recv(localFd, &c, 1, 0) <= 0) {
                        cout << "Msg Char Receive Fails" << endl;
                        return temp;
                    }

                    // cout << "RECEIVE CHAR" << endl;
                    // cout << c << endl;

                    char* p = new char;
                    *p = c;

                    args[i] = (void*)p;
                } else {
                    // cout << "RECEIVE CHAR ARRAY" << endl;

                    // char array
                    char* p = new char[argLength];
                    // char char_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        char c;
                        if (temp = recv(localFd, &c, 1, 0) <= 0) {
                            cout << "Msg Char Array Receive Fails" << endl;
                            return temp;
                        }

                        // cout << c << endl;

                        *(p + j) = c;
                        // char_array[j] = c;
                    }

                    args[i] = (void*)p;
                }
            }
            break;
            case ARG_SHORT: {
                if (argLength == 0) {
                    // scalar
                    short s;
                    if (temp = recv(localFd, &s, 2, 0) <= 0) {
                        cout << "Msg Short Receive Fails" << endl;
                        return temp;
                    }

                    // cout << "RECEIVE SHORT" << endl;
                    // cout << s << endl;

                    short* p = new short;
                    *p = s;

                    args[i] = (void*)p;
                } else {
                    // short array
                    short* p = new short[argLength];
                    // short short_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        short s;
                        if (temp = recv(localFd, &s, 2, 0) <= 0) {
                            cout << "Msg Short Array Receive Fails" << endl;
                            return temp;
                        }

                        *(p + j) = s;
                        // short_array[j] = s;
                    }
                    args[i] = (void*)p;
                }
            }
            break;
            case ARG_INT: {
                if (argLength == 0) {
                    // scalar
                    int in;
                    if (temp = recv(localFd, &in, 4, 0) <= 0) {
                        cout << "Msg Int Receive Fails" << endl;
                        return temp;
                    }

                    // cout << "RECEIVE INT" << endl;
                    // cout << in << endl;

                    int* p = new int;
                    *p = in;

                    args[i] = (void*)p;

                    // cout << i << endl;
                    // cout << *((int*)(void*)&in) << endl;
                    // args[i] = (void*)&in;
                } else {
                    // int array
                    int* p = new int[argLength];
                    // int int_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        int in;
                        if (temp = recv(localFd, &in, 4, 0) <= 0) {
                            cout << "Msg Int Array Receive Fails" << endl;
                            return temp;
                        }

                        // cout << "RECEIVE ARG" << endl;
                        // cout << in2 << endl;

                        *(p + j) = in;
                        // int_array[j] = in2;
                    }

                    args[i] = (void*)p;
                }
            }
            break;
            case ARG_LONG: {
                if (argLength == 0) {
                    // scalar
                    long l;
                    if (temp = recv(localFd, &l, 4, 0) <= 0) {
                        cout << "Msg Long Receive Fails" << endl;
                        return temp;
                    }
                    
                    // cout << "RECEIVE LONG" << endl;
                    // cout << l << endl;

                    long* p = new long;
                    *p = l;

                    args[i] = (void*)p;
                } else {
                    // long array
                    long* p = new long[argLength];
                    // long long_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        long l;
                        if (temp = recv(localFd, &l, 4, 0) <= 0) {
                            cout << "Msg Long Array Receive Fails" << endl;
                            return temp;
                        }

                        *(p + j) = l;
                        // long_array[j] = l2;
                    }
                    
                    args[i] = (void*)p;
                }
            }
            break;
            case ARG_DOUBLE: {
                if (argLength == 0) {
                    // scalar
                    double d;
                    if (temp = recv(localFd, &d, 8, 0) <= 0) {
                        cout << "Msg Double Receive Fails" << endl;
                        return temp;
                    }

                    double* p = new double;
                    *p = d;

                    args[i] = (void*)p;
                } else {
                    // double array
                    double* p = new double[argLength];
                    // double double_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        double d;
                        if (temp = recv(localFd, &d, 8, 0) <= 0) {
                            cout << "Msg Double Array Receive Fails" << endl;
                            return temp;
                        }

                        *(p + j) = d;
                        // double_array[j] = d;
                    }
                    args[i] = (void*)p;
                }
            }
            break;
            case ARG_FLOAT: {
                if (argLength == 0) {
                    // scalar
                    float fl;
                    if (temp = recv(localFd, &fl, 4, 0) <= 0) {
                        cout << "Msg Float Receive Fails" << endl;
                        return temp;
                    }

                    float* p = new float;
                    *p = fl;

                    args[i] = (void*)p;
                } else {
                    // char array
                    float* p = new float[argLength];
                    // float float_array[argLength];
                    for (int j = 0; j < argLength; j++) {
                        float fl;
                        if (temp = recv(localFd, &fl, 4, 0) <= 0) {
                            cout << "Msg Float Array Receive Fails" << endl;
                            return temp;
                        }

                        *(p + j) = fl;
                        // float_array[j] = fl2;
                    }
                    args[i] = (void*)p;
                }
            }
            break;
            default:
            break;
        }
        
    }

    // cout << "***********" << endl;
    // for (int j = 0; j < argTypesLength-1; j++) {
    //     cout << *((int*)args[j]) << endl;
    // }

    return 0;
}

int sendExeResSuccess(int localFd, string name, int argTypes[], void** args) {
    unsigned int nameSize = name.size() + 1;

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    unsigned int argTypesSize = argTypesLength * 4;

    int temp;
    int type = EXECUTE_SUCCESS;
    // send msg type
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg LOC_REQUEST Sending Fails" << endl;
        return temp;
    } 
    // send program name size
    if (temp = send(localFd, &nameSize, 4, 0) < 0) {
        cout << "Msg name size Sending Fails" << endl;
        return temp;
    } 
    // send program name
    // cout << name << endl;
    if (temp = send(localFd, name.c_str(), nameSize, 0) < 0) {
        cout << "Msg name Sending Fails" << endl;
        return temp;
    } 
    // send argTypes size
    if (temp = send(localFd, &argTypesSize, 4, 0) < 0) {
        cout << "Msg argTypes Sending Fails" << endl;
        return temp;
    } 
    // send argTypes 
    for (int i = 0; i < argTypesLength; i++) {
        // cout << argTypes[i] << endl;
        if (temp = send(localFd, &argTypes[i], 4, 0) < 0) {
            cout << "Msg argTypes Sending Fails" << endl;
            return temp;
        }
    }

    // send args
    for (unsigned int i = 0; i < argTypesLength - 1; i++) {
        int argType = argTypes[i];

        // get length of this argument
        int argLength = (int)(argType & 65535);
        argLength = argLength? argLength: 1;
        // get type of this argument
        int type = ((argType >> 16) & 255);
        
        void* arg = args[i];

        // cout << "TYPE" << endl;
        // cout << type << endl;

        switch(type) {
            case ARG_CHAR: {
                char* chars = (char *)arg;
                for (int j = 0; j < argLength; j++) {
                    char c = *(chars + j);
                    if (temp = send(localFd, &c, 1, 0) < 0) {
                        cout << "Msg char array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND CHAR" << endl;
                    // cout << c << endl;
                }
                
            }
            break;
            case ARG_SHORT: {
                short* shortInts = (short*)arg;
                for (int j = 0; j < argLength; j++) {
                    short si = *(shortInts + j);
                    if (temp = send(localFd, &si, 2, 0) < 0) {
                        cout << "Msg short array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND SHORT" << endl;
                    // cout << si << endl;
                }
            }
            break;
            case ARG_INT: {
                // cout << "SEND INT ARG" << endl;
                int* Ints = (int*)arg;
                for (int j = 0; j < argLength; j++) {
                    int in = *(Ints + j);

                    // cout << "SEND ARG" << endl;
                    // cout << in << endl;

                    if (temp = send(localFd, &in, 4, 0) < 0) {
                        cout << "Msg int array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND INT" << endl;
                    // cout << in << endl;
                }
            }
            break;
            case ARG_LONG: {
                long* longInts = (long*)arg;
                for (int j = 0; j < argLength; j++) {
                    long li = *(longInts + j);
                    if (temp = send(localFd, &li, 4, 0) < 0) {
                        cout << "Msg long array Sending Fails" << endl;
                        return temp;
                    }

                    // cout << "SEND LONG" << endl;
                    // cout << li << endl;
                }
            }
            break;
            case ARG_DOUBLE: {
                double* doubleInts = (double*)arg;
                for (int j = 0; j < argLength; j++) {
                    double d = *(doubleInts + j);
                    if (temp = send(localFd, &d, 8, 0) < 0) {
                        cout << "Msg double array Sending Fails" << endl;
                        return temp;
                    }
                }
            }
            break;
            case ARG_FLOAT: {
                float* floats = (float*)arg;
                for (int j = 0; j < argLength; j++) {
                    float f = *(floats + j);
                    if (temp = send(localFd, &f, 4, 0) < 0) {
                        cout << "Msg float array Sending Fails" << endl;
                        return temp;
                    }
                } 
            }
            break;
            default:
            break;
        }
    }

    return 0;
}

int recvExeReqSuccess(int localFd, char* &name, int* &argTypes, void** &args) {
    return recvExeReq(localFd, name, argTypes, args);
}

int sendExeResFailure(int localFd, ReasonCode code) {
    int temp;
    int type = EXECUTE_FAILURE;

    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg EXECUTE_FAILURE Sending Fails" << endl;
        return temp;
    }
    if (temp = send(localFd, &code, 4, 0) < 0) {
        cout << "Msg Error Code Sending Fails" << endl;
        return temp;
    } 
    return 0;
}

int recvExeReqFailure(int localFd, ReasonCode &reasonCode) {
    int temp, temp2;

    if (temp = recv(localFd, &temp2, 4, 0) <= 0) {
        cout << "Msg ExeReq ReasonCode Receive Fails" << endl;
        return temp;
    } 
    reasonCode = (ReasonCode)temp2;
    return 0;
}
/****************************************************************/

/**********************TERMINATE REQUEST*************************/
int sendTermReq(int localFd) {
    int temp;
    int type = TERMINATE;
    if (temp = send(localFd, &type, 4, 0) < 0) {
        cout << "Msg TERMINATE Sending Fails" << endl;
        return temp;
    } 
    return 0;
}
/****************************************************************/
