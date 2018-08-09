#include "rpc.h"
#include "socket.h"
#include "reasonCode.h"

#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <set>
#include <string.h>
#include <map>
#include <vector>
#include <unistd.h>
#include <list>

using namespace std;

struct functionIdentifier {
    string name;
    int* argTypes;

    functionIdentifier(string name, int *argTypes) : name(name), argTypes(argTypes){}
};

bool operator < (const functionIdentifier &l, const functionIdentifier &r) {

    if(l.name == r.name) {
		unsigned int ll = 0;
		unsigned int lr = 0;

		while (l.argTypes[ll++]);
		while (r.argTypes[lr++]);

		// cout << ll << endl;
		// cout << lr << endl;
		// cout << "------------" << endl;

		if (ll == lr) {
			return false;
		} else if (ll > lr) {
			return false;
		} else {
			return true;
		}
    }
    else {
        return (l.name < r.name);
    }
}

struct serverIdentifier {
    string server_identifier;
    unsigned short port;

    serverIdentifier(string server_identifier, unsigned short port) : server_identifier(server_identifier), port(port) {}
};

bool operator == (const serverIdentifier &l, const serverIdentifier &r) {
    return (l.server_identifier == r.server_identifier && l.port == r.port);
}

static int serverSocketFd;
static int binderSocketFd;

static bool clientConnectedToBinder = false;
static int clientBinderSocketFd;

static set<pthread_t> allExecutThreads;

static bool shouldTerminate = false;
static map<functionIdentifier, skeleton> registeredFunctions;

static map<functionIdentifier, list<serverIdentifier*>*> cachedFunctions;

struct threadArgs {
	int clientSocketFd;
	string name;
	int* argTypes;
	void** args;
};

int rpcInit() {
    char* binderAddress = getenv ("BINDER_ADDRESS");
    char* binderPort = getenv("BINDER_PORT");

    if(binderAddress == NULL || binderPort == NULL) {
		return INIT_UNSET_BINDER_ADDRESS;
	}

	serverSocketFd = createSocket();
	bindAddress(serverSocketFd);
    socketListen(serverSocketFd);

	binderSocketFd = createSocket();
    socketConnectTo(binderSocketFd, binderAddress, atoi(binderPort));

    return 0;
}

int rpcCall(char* name, int* argTypes, void** args) {
	int status;

	if (!clientConnectedToBinder) {
		char* binderAddressString = getenv ("BINDER_ADDRESS");
		char* binderPortString = getenv("BINDER_PORT");

		if(binderAddressString == NULL || binderPortString == NULL) {
			return INIT_UNSET_BINDER_ADDRESS;
		}

		clientBinderSocketFd = createSocket();
		socketConnectTo(clientBinderSocketFd, binderAddressString, atoi(binderPortString));
		if (clientBinderSocketFd < 0) {
			return INIT_BINDER_SOCKET_FAILURE;
		}

		/*****************sendLocReq********************/
		// sendLocReq(int localFd, string name, int argTypes[])
		status = sendLocReq(clientBinderSocketFd, string(name), argTypes);
		if (status < 0) {
			return status;
		}
    } else {
		/*****************sendLocReq********************/
		// sendLocReq(int localFd, string name, int argTypes[])
		status = sendLocReq(clientBinderSocketFd, string(name), argTypes);
		if (status < 0) {
			return status;
		}
	}

	/*****************recvLocReqSuccess/recvLocReqFailure********************/
	char* serverID;
	unsigned short port;

	MessageType messageType;
	status = recvMsgType(clientBinderSocketFd, messageType);
	if (status == 0) {
		// cout << type << endl;
		ReasonCode reasoncode;
		if (messageType == LOC_SUCCESS) {
			
			/*****************recvRegReq********************/
			// recvLocReqSuccess(int localFd, char* serverID, unsigned short &port)
			recvLocReqSuccess(clientBinderSocketFd, serverID, port);

			// cout << "************************" << endl;
			// cout << serverID << endl;
			// cout << port << endl;
			// cout << "************************" << endl;

			// reasoncode should be > 0;
		} else if (messageType == LOC_FAILURE) {
			
			status = recvLocReqFailure(clientBinderSocketFd, reasoncode);

			return reasoncode;
		}
	} else {
		return status;
	}

	int clientserverSocketFd = createSocket();
    socketConnectTo(clientserverSocketFd, serverID, (int)port);
	if (status < 0) {
		return status;
	}

	/*****************sendExeReq********************/
	// sendExeReq(int localFd, string name, int argTypes[], void**args)
	status = sendExeReq(clientserverSocketFd, string(name), argTypes, args);
	if (status < 0) {
		return status;
	}

	// MessageType messageType;
	status = recvMsgType(clientserverSocketFd, messageType);
	if (status == 0) {
		// cout << "MESSAGE TYPE" << endl;
		// cout << messageType << endl;

		ReasonCode reasoncode;
		if (messageType == EXECUTE_SUCCESS) {
			
			/*****************recvExeReqSuccess********************/
			// recvExeReqSuccess(int localFd, char* &name, int* &argTypes, void** &args)

			// char* name;
			// int* argTypes;
			// void** args;
			recvExeReqSuccess(clientserverSocketFd, name, argTypes, args);
			
			// cout << "************************" << endl;
			// cout << name << endl;
			// unsigned int leng = 0;
			// while (argTypes[leng++]);
			// cout << leng << endl;
			// for (int j = 0; j < leng; j++) {
			// 	cout << argTypes[j] << endl;
			// }

			// // f0 -> colrect

			// // f1 -> long doesn't work
			// // char a1 = 'a';
			// // short b1 = 100;
			// // int c1 = 1000;
			// // long d1 = 10000;
			// cout << *((long*)args[0]) << endl;
			// cout << *((char*)args[1]) << endl;
			// cout << *((short*)args[2]) << endl;
			// cout << *((int*)args[3]) << endl;
			// cout << *((long*)args[4]) << endl;

			// // f2 -> colrect
			// // float a2 = 3.14159;
			// // double b2 = 1234.1001;
			// cout << (char*)args[0] << endl;
			// cout << *((float*)args[1]) << endl;
			// cout << *((double*)args[2]) << endl;
			// cout << "************************" << endl;

			// reasoncode should be > 0;
		} else {
			/*****************recvExeReqFailure********************/
			// recvExeReqFailure(int localFd, ReasonCode reasonCode)

			// cout << "EXECUTION FAILED RECEIVE REASONCODE" << endl;
			status = recvExeReqFailure(clientserverSocketFd, reasoncode);
			// cout << reasoncode << endl;
			return reasoncode;
		}
	} else {
		return status;
	}

    close(clientserverSocketFd);

    return status;
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
    int status;

	// Scan cachedFunctions
	functionIdentifier k(string(name), argTypes);

    if (cachedFunctions.find(k) != cachedFunctions.end()) { // Cacaed

		// cout << "CACHED" << endl;

        list<serverIdentifier*>* availableServers = cachedFunctions[k];

		for (list<serverIdentifier*>::iterator it = availableServers->begin(); it != availableServers->end(); it++) {
			serverIdentifier* serverHasFunction = (*it);

			MessageType messageType;
			string serverID = serverHasFunction->server_identifier;
			unsigned short port = serverHasFunction->port;
			
			int clientserverSocketFd = createSocket();
			socketConnectTo(clientserverSocketFd, serverID.c_str(), (int)port);
			if (status < 0) {
				return status;
			}

			/*****************sendExeReq********************/
			// sendExeReq(int localFd, string name, int argTypes[], void**args)
			status = sendExeReq(clientserverSocketFd, string(name), argTypes, args);
			if (status < 0) {
				return status;
			}

			// MessageType messageType;
			status = recvMsgType(clientserverSocketFd, messageType);
			if (status == 0) {
				// cout << "MESSAGE TYPE" << endl;
				// cout << messageType << endl;

				ReasonCode reasoncode;
				if (messageType == EXECUTE_SUCCESS) {
					
					/*****************recvExeReqSuccess********************/
					// recvExeReqSuccess(int localFd, char* &name, int* &argTypes, void** &args)

					// char* name;
					// int* argTypes;
					// void** args;
					recvExeReqSuccess(clientserverSocketFd, name, argTypes, args);
					
					// cout << "************************" << endl;
					// cout << name << endl;
					// unsigned int leng = 0;
					// while (argTypes[leng++]);
					// cout << leng << endl;
					// for (int j = 0; j < leng; j++) {
					// 	cout << argTypes[j] << endl;
					// }

					// // f0 -> colrect

					// // f1 -> long doesn't work
					// // char a1 = 'a';
					// // short b1 = 100;
					// // int c1 = 1000;
					// // long d1 = 10000;
					// cout << *((long*)args[0]) << endl;
					// cout << *((char*)args[1]) << endl;
					// cout << *((short*)args[2]) << endl;
					// cout << *((int*)args[3]) << endl;
					// cout << *((long*)args[4]) << endl;

					// // f2 -> colrect
					// // float a2 = 3.14159;
					// // double b2 = 1234.1001;
					// cout << (char*)args[0] << endl;
					// cout << *((float*)args[1]) << endl;
					// cout << *((double*)args[2]) << endl;
					// cout << "************************" << endl;

					// reasoncode should be > 0;
					return 0;
				} else {
					/*****************recvExeReqFailure********************/
					// recvExeReqFailure(int localFd, ReasonCode reasonCode)

					// cout << "EXECUTION FAILED RECEIVE REASONCODE" << endl;
					status = recvExeReqFailure(clientserverSocketFd, reasoncode);
					// cout << reasoncode << endl;
					// return reasoncode;
				}
			} else {
				return status;
			}

			close(clientserverSocketFd);
		}
    }

	// cout << "NOT CACHED" << endl;

	if (!clientConnectedToBinder) {
		char* binderAddressString = getenv ("BINDER_ADDRESS");
		char* binderPortString = getenv("BINDER_PORT");

		if(binderAddressString == NULL || binderPortString == NULL) {
			return INIT_UNSET_BINDER_ADDRESS;
		}

		clientBinderSocketFd = createSocket();
		socketConnectTo(clientBinderSocketFd, binderAddressString, atoi(binderPortString));
		if (clientBinderSocketFd < 0) {
			return INIT_BINDER_SOCKET_FAILURE;
		}

		/*****************sendCacheReq********************/
		// sendLocReq(int localFd, string name, int argTypes[])
		status = sendCacheReq(clientBinderSocketFd, string(name), argTypes);
		if (status < 0) {
			return status;
		}
    } else {
		/*****************sendCacheReq********************/
		// sendLocReq(int localFd, string name, int argTypes[])
		status = sendCacheReq(clientBinderSocketFd, string(name), argTypes);
		if (status < 0) {
			return status;
		}
	}

	// cout << "CONNECTED TO BINDER" << endl;

	// Take a copy of argTypes
	unsigned int argTypesLength = 0;
	while (argTypes[argTypesLength++]);
	int* argTypesCopy = new int[argTypesLength];

	unsigned int index = 0;
	while (argTypes[index] != 0) {
		argTypesCopy[index] = argTypes[index];
		index++;
	}
	argTypesCopy[index] = 0;

	// Check if its already cached
	functionIdentifier key = functionIdentifier(name, argTypesCopy);
	if (cachedFunctions.find(key) == cachedFunctions.end()) {
		cachedFunctions[key] = new list<serverIdentifier*>();
	}

	ReasonCode r = SUCCESS;

	list<serverIdentifier*>* serverList = cachedFunctions[key];

	if (serverList->size()) { // empty the list
		serverList->pop_back();
	}

	/*****************recvCacheNumber********************/
	int cacheNumber;
	status = recvCacheNumber(clientBinderSocketFd, cacheNumber);
	/****************************************************/

	/*****************recvLocReqSuccess/recvLocReqFailure********************/
	MessageType messageType;
	char* serverID;
	unsigned short port;

	while (cacheNumber) {
		
		status = recvMsgType(clientBinderSocketFd, messageType);
		if (status == 0) {
			// cout << type << endl;
			ReasonCode reasoncode;
			if (messageType == LOC_SUCCESS) {
				
				/*****************recvCacheReqSuccess********************/
				// recvCacheReqSuccess(int localFd, char* serverID, unsigned short &port)
				recvCacheReqSuccess(clientBinderSocketFd, serverID, port);

				// cout << "************************" << endl;
				// cout << serverID << endl;
				// cout << port << endl;
				// cout << "************************" << endl;

				serverIdentifier* id = new serverIdentifier(serverID, port);
				serverList->push_back(id);

			} else if (messageType == LOC_FAILURE) {
				
				status = recvCacheReqFailure(clientBinderSocketFd, reasoncode);

				return reasoncode;
			}
		} else {
			return status;
		}

		cacheNumber--;
	}
	
	rpcCacheCall(name, argTypes, args);

    return status;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
	int status;

	char localHostName[256];
	gethostname(localHostName, 256);
	string hostname = string(localHostName);

	struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(serverSocketFd, (struct sockaddr *)&sin, &len);
	unsigned short port = ntohs(sin.sin_port);

	/*****************sendRegReq********************/
	// sendRegReq(int localFd, string server_identifier, unsigned short port, string name, int argTypes[])
	status = sendRegReq(binderSocketFd, hostname, port, string(name), argTypes);
	if (status < 0) {
		return status;
	}

	/*****************recvRegReqSuccess/recvRegReqFailure********************/
	MessageType messageType;
	status = recvMsgType(binderSocketFd, messageType);
	if (status == 0) {
		// cout << type << endl;
		ReasonCode reasoncode;
		if (messageType == REGISTER_SUCCESS) {

			status = recvRegReqSuccess(binderSocketFd, reasoncode);

			/*********LOCAL DATABASE**********/
			struct functionIdentifier k(string(name), argTypes);

			// cout << "************REGISTER************" << endl;
			// cout << registeredFunctions.count(k) << endl;
			
			// cout << name << endl;
			// unsigned int leng = 0;
			// while (argTypes[leng++]);
			// // cout << leng << endl;
			// for (int j = 0; j < leng; j++) {
			// 	cout << argTypes[j] << endl;
			// }
			// cout << "***********************************" << endl;

			registeredFunctions[k] = f;
			/*********************************/

			return reasoncode;
		} else if (messageType == REGISTER_FAILURE) {
			
			status = recvRegReqFailure(binderSocketFd, reasoncode);

			return reasoncode;
		}
	} else {
		return status;
	}

	return 0;
}

void* threadFunction(void* tArgs) {
	int status;
	
	struct threadArgs* threadArgs = (struct threadArgs*)tArgs;

	int clientSocketFd = threadArgs->clientSocketFd;
	string name = threadArgs->name;
	int* argTypes = threadArgs->argTypes;
	void** args = threadArgs->args;

	// cout << "EXECUTE FUNCTION" << endl;
	// cout << name << endl;
	
	// Find the function that we want to EXECUTE
	struct functionIdentifier k(name, argTypes);
	skeleton skel = registeredFunctions[k];

	// cout << "************EXECUTE SKEL************" << endl;
	// cout << registeredFunctions.count(k) << endl;
	// cout << "***********************************" << endl;

	// cout << "FUNCTION FOUND" << endl;
	
	// cout << "************************" << endl;
	// cout << name << endl;
	// unsigned int leng = 0;
	// while (argTypes[leng++]);
	// // cout << leng << endl;
	// for (int j = 0; j < leng; j++) {
	// 	cout << argTypes[j] << endl;
	// }

	// // f0 -> colrect

	// // f1 -> long doesn't work
	// // char a1 = 'a';
	// // short b1 = 100;
	// // int c1 = 1000;
	// // long d1 = 10000;
	// cout << *((long*)args[0]) << endl;
	// cout << *((char*)args[1]) << endl;
	// cout << *((short*)args[2]) << endl;
	// cout << *((int*)args[3]) << endl;
	// cout << *((long*)args[4]) << endl;

	// // f2 -> colrect
	// // float a2 = 3.14159;
	// // double b2 = 1234.1001;
	// cout << (char*)args[0] << endl;
	// cout << *((float*)args[1]) << endl;
	// cout << *((double*)args[2]) << endl;
	// cout << "************************" << endl;

	int executionResult = skel(argTypes, args);

	// cout << "EXECUTE STATUS" << endl;
	// cout << executionResult << endl; 

	if (executionResult == 0) {

		/*****************sendExeResSuccess********************/
		// sendExeResSuccess(int localFd, string name, int argTypes[], void** args)
		status = sendExeResSuccess(clientSocketFd, string(name), argTypes, args);
		if (status < 0) {
			return NULL;
		}

	} else {
		/*****************sendExeResFailure********************/
		// sendExeResFailure(int localFd, ReasonCode code)

		// cout << "EXECUTION FAILED SEND REASONCODE" << endl;
		ReasonCode r = EXECUTION_FAILED;
		// cout << r << endl;
        status = sendLocResFailure(clientSocketFd, r);
	}

	return NULL;
}

int rpcExecute() {
	if (registeredFunctions.size() == 0) return -1;

	// cout << "NUMBER OF REGISTERED FUNCTIONS" << endl;
	// cout << registeredFunctions.size() << endl;

    int max_fd = serverSocketFd;

    fd_set master_set;
	fd_set working_set;

    FD_ZERO(&master_set);
	FD_ZERO(&working_set);

    FD_SET(serverSocketFd, &master_set);
    FD_SET(binderSocketFd, &master_set);

    while (!shouldTerminate) {
		working_set = master_set;

		if (select(max_fd + 1, &working_set, NULL, NULL, NULL) < 0) {
			return SELECT_FAILED;
		}

		for (int i = 0; i <= max_fd; i++) {

			if (FD_ISSET(i, &working_set)) {
				if (i == serverSocketFd) {
					int newSocketFd = socketAccept(serverSocketFd);

					FD_SET(newSocketFd, &master_set);
					if(newSocketFd > max_fd) {
						max_fd = newSocketFd;
					}
				} else {
					MessageType messageType;
                    if (recvMsgType(i, messageType) == 0) {
                        if (messageType == EXECUTE) {
							
							char* name;
							int* argTypes;
							void** args;
							recvExeReq(i, name, argTypes, args);
							
							// cout << "************************" << endl;

							// cout << name << endl;
							// unsigned int leng = 0;
							// while (argTypes[leng++]);
							// // cout << leng << endl;
							// for (int j = 0; j < leng; j++) {
							// 	cout << argTypes[j] << endl;
							// }

							// // f0 -> colrect

							// // f1 -> long doesn't work
							// // char a1 = 'a';
							// // short b1 = 100;
							// // int c1 = 1000;
							// // long d1 = 10000;
							// // cout << *((long*)args[0]) << endl;
							// // cout << *((char*)args[1]) << endl;
							// // cout << *((short*)args[2]) << endl;
							// // cout << *((int*)args[3]) << endl;
							// // cout << *((long*)args[4]) << endl;

							// // f2 -> colrect
							// // float a2 = 3.14159;
							// // double b2 = 1234.1001;
							// // cout << (char*)args[0] << endl;
							// // cout << *((float*)args[1]) << endl;
							// // cout << *((double*)args[2]) << endl;

							// // f3 -> long doesn't work
							// // long a3[11] = {11, 109, 107, 105, 103, 101, 102, 104, 106, 108, 110};

							// cout << "************************" << endl;

							struct threadArgs threadArgs;
							threadArgs.clientSocketFd = i;
							threadArgs.name = string(name);
							threadArgs.argTypes = argTypes;
							threadArgs.args = args;

							pthread_t sendingThread;
							pthread_create(&sendingThread, NULL, &threadFunction, &threadArgs);
							allExecutThreads.insert(sendingThread);
							// _runningThreads[sendingThread] = true;

                        } else if (messageType == TERMINATE) {
							// Authentication
							if (i == binderSocketFd) {
								shouldTerminate = true;
							}
                            
							break;
                        }
                    } else {  // elror on receive message from client
						close(i);
						FD_CLR(i, &master_set);
					}
				}
			}
		}
    }

    vector<pthread_t> t;

	for (set<pthread_t>::iterator it = allExecutThreads.begin(); it != allExecutThreads.end(); ++it) {
		pthread_join(*it, NULL);
	}

    close(binderSocketFd);
    close(serverSocketFd);

	return 0;
}

int rpcTerminate() {
	if (!clientConnectedToBinder) {
		char* binderAddressString = getenv ("BINDER_ADDRESS");
		char* binderPortString = getenv("BINDER_PORT");

		if(binderAddressString == NULL || binderPortString == NULL) {
			return INIT_UNSET_BINDER_ADDRESS;
		}

		clientBinderSocketFd = createSocket();
		socketConnectTo(clientBinderSocketFd, binderAddressString, atoi(binderPortString));

		if (clientBinderSocketFd < 0) {
			return INIT_BINDER_SOCKET_FAILURE;
		} else {
			sendTermReq(clientBinderSocketFd);
		}
    } else {
		sendTermReq(clientBinderSocketFd);
	}
}
