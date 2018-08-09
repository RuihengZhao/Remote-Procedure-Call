#include "binder.h"

using namespace std;

bool operator == (const serverIdentifier &l, const serverIdentifier &r) {
    return (l.server_identifier == r.server_identifier && l.port == r.port);
}

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

// <<name, argTypes>, list<<serverID, port>>
static map<functionIdentifier, list<serverIdentifier*>*> registeredFunctions;

// All Servers connected
// <SocketFd, <serverID, port>>
static map<int, serverIdentifier*> connectedServers;

// list<<serverID, port>>
static list<serverIdentifier*> roundRobinList;

static bool binderTerminate;

void handleRegisterRequest(int i) {
    /*****************recvRegReq********************/
    // recvRegReq(int localFd, char* serverID, unsigned short &port, char* name, int* argTypes)
    char* serverID;
    unsigned short port;
    char* name;
    int* argTypes;
    
    recvRegReq(i, serverID, port, name, argTypes);

    // cout << "***********REGISTER*************" << endl;
    // cout << serverID << endl;
    // cout << port << endl;
    // cout << name << endl;
    // unsigned int leng = 0;
    // while (argTypes[leng++]);
    // cout << leng << endl;
    // for (int j = 0; j < leng; j++) {
    //     cout << argTypes[j] << endl;
    // }
    // cout << "********************************" << endl;

    bool serverRegistered = false;
    serverIdentifier* identifier = new serverIdentifier(serverID, port);
    for (list<serverIdentifier* >::iterator it = roundRobinList.begin(); it != roundRobinList.end(); it++) {
        serverIdentifier* si = *it;

        if (*si == *identifier) {
            serverRegistered = true;
            break;
        }
    }

    if (!serverRegistered) {
        roundRobinList.push_back(identifier);
        connectedServers[i] = identifier;
    }

    unsigned int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int* argTypesCopy = new int[argTypesLength];

    unsigned int index = 0;
    while (argTypes[index] != 0) {
        argTypesCopy[index] = argTypes[index];
        index++;
    }
    argTypesCopy[index] = 0;

    functionIdentifier key = functionIdentifier(name, argTypesCopy);

    if (registeredFunctions.find(key) == registeredFunctions.end()) {
        registeredFunctions[key] = new list<serverIdentifier*>();
    }

    ReasonCode r = SUCCESS;

    list<serverIdentifier*>* serverList = registeredFunctions[key];
    serverIdentifier* id = new serverIdentifier(serverID, port);

    bool functionRegistered = false;
    for (list<serverIdentifier*>::iterator it = serverList->begin(); it != serverList->end(); it++) {
        serverIdentifier* si = *it;

        if (*si == *id) {
            functionRegistered = true;
            r = FUNCTION_OVERRIDDEN;
        }
    }

    if (!functionRegistered) {
        serverList->push_back(id);
    }

    int status = sendRegResSuccess(i, r);
}

void handleLocRequest(int i) {
    int status;

    /*****************recvRegReq********************/
    // recvLocReq(int localFd, char* name, int* argTypes)

    char* name;
    int* argTypes;
    
    recvLocReq(i, name, argTypes);
    
    // cout << "***********LOCATION*************" << endl;
    // cout << name << endl;
    // unsigned int leng = 0;
    // while (argTypes[leng++]);
    // cout << leng << endl;
    // for (int j = 0; j < leng; j++) {
    //     cout << argTypes[j] << endl;
    // }
    // cout << "*******************************" << endl;

    functionIdentifier key(string(name), argTypes);

    ReasonCode r = SUCCESS;
    
    if (registeredFunctions.find(key) == registeredFunctions.end()) {

        /*****************sendLocResFailure********************/
        /******************************************************/
        // sendLocResFailure(int localFd, ReasonCode code)
        r = FUNCTION_NOT_AVAILABLE;
        status = sendLocResFailure(i, r);

    } else {
        list<serverIdentifier*>* availableServers = registeredFunctions[key];
        serverIdentifier* selectedServer = NULL;

        bool serverFound = false;
        for (unsigned int i = 0; i < roundRobinList.size(); i++) {
            serverIdentifier* nextServerInTurn = roundRobinList.front();

            for (list<serverIdentifier*>::iterator it = availableServers->begin(); it != availableServers->end(); it++) {
                serverIdentifier* serverHasFunction = (*it);
                
                if (*nextServerInTurn == *serverHasFunction) {
                    selectedServer = nextServerInTurn;
                    serverFound = true;
                    break;
                }
            }

            roundRobinList.splice(roundRobinList.end(), roundRobinList, roundRobinList.begin());

            if (serverFound) break;
        }

        if (selectedServer != NULL) {

            /*****************sendLocResSuccess********************/
            /******************************************************/
            // sendLocResSuccess(int localFd, string server_identifier, unsigned short port)
            r = SUCCESS;
            string server_identifier = selectedServer->server_identifier;
            unsigned short port = selectedServer->port;
            
            status = sendLocResSuccess(i, server_identifier, port);
        
        } else {

            /*****************sendLocResFailure********************/
            /******************************************************/
            // sendLocResFailure(int localFd, ReasonCode code)
            r = FUNCTION_NOT_AVAILABLE;
            status = sendLocResFailure(i, r);
        
        }
    }
}

void handleTerminateRequest() {
    for (map<int, serverIdentifier* >::iterator it = connectedServers.begin(); it != connectedServers.end(); it++) {
        sendTermReq(it->first);
    }

    // list<int> removeList;
    int totalNumOfServers = connectedServers.size();
    int terminatedServers = 0;
    int connectStatus = 0;

    while (true) {
        for (map<int, serverIdentifier* >::iterator it = connectedServers.begin(); it != connectedServers.end(); it++) {
            int serverFd = it->first;
            serverIdentifier* server = it->second;

            int socketFd = createSocket();
            connectStatus = socketConnectTo(socketFd, server->server_identifier.c_str(), server->port);

            if (connectStatus < 0) {
                terminatedServers++;
            }
        }

        if (terminatedServers == totalNumOfServers) break;

        sleep(1);
    }

    binderTerminate = true;
}

void handleCacheRequest(int i) {
    int status;

    /*****************recvRegReq********************/
    // recvLocReq(int localFd, char* name, int* argTypes)

    char* name;
    int* argTypes;
    
    recvCacheReq(i, name, argTypes);
    
    // cout << "***********CACHE*************" << endl;
    // cout << name << endl;
    // unsigned int leng = 0;
    // while (argTypes[leng++]);
    // cout << leng << endl;
    // for (int j = 0; j < leng; j++) {
    //     cout << argTypes[j] << endl;
    // }
    // cout << "*******************************" << endl;

    functionIdentifier key(string(name), argTypes);

    ReasonCode r = SUCCESS;
    
    if (registeredFunctions.find(key) == registeredFunctions.end()) {

        /*****************sendLocResFailure********************/
        /******************************************************/
        // sendLocResFailure(int localFd, ReasonCode code)
        r = FUNCTION_NOT_AVAILABLE;
        status = sendLocResFailure(i, r);

    } else {
        list<serverIdentifier*>* availableServers = registeredFunctions[key];

        if (availableServers != NULL) {

            int numberOfAvailableServers = availableServers->size();
            status = sendCacheNumber(i, numberOfAvailableServers);

            for (list<serverIdentifier*>::iterator it = availableServers->begin(); it != availableServers->end(); it++) {
                serverIdentifier* serverHasFunction = (*it);
                
                /*****************sendCacheResSuccess********************/
                /******************************************************/
                // sendCacheResSuccess(int localFd, string server_identifier, unsigned short port)
                r = SUCCESS;
                string server_identifier = serverHasFunction->server_identifier;
                unsigned short port = serverHasFunction->port;
                
                status = sendCacheResSuccess(i, server_identifier, port);
            }

        } else {

            /*****************sendLocResFailure********************/
            /******************************************************/
            // sendLocResFailure(int localFd, ReasonCode code)
            r = FUNCTION_NOT_AVAILABLE;
            status = sendCacheResFailure(i, r);
        
        }
    }
}

int runBinder() {
    int localSocketFd = createSocket();
    bindAddress(localSocketFd);
    socketListen(localSocketFd);

    char localHostName[256];
    gethostname(localHostName, 256);

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(localSocketFd, (struct sockaddr *)&sin, &len);

    cout << "BINDER_ADDRESS " << localHostName << endl;
    cout << "BINDER_PORT " << ntohs(sin.sin_port) << endl;

    int max_fd = localSocketFd;

    fd_set master_set;
    fd_set working_set;

    FD_ZERO(&master_set);
	FD_ZERO(&working_set);

    FD_SET(localSocketFd, &master_set);

    binderTerminate = false;
    while (!binderTerminate) {
        working_set = master_set;

        if (select(max_fd + 1, &working_set, NULL, NULL, NULL) < 0) {
			return SELECT_FAILED;
		}

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &working_set)) {
                if (i == localSocketFd) {
                    int newSocketFd = socketAccept(localSocketFd);

                    FD_SET(newSocketFd, &master_set);
					if(newSocketFd > max_fd) {
						max_fd = newSocketFd;
					}
                } else {
                    if (binderTerminate) {
                        break;
                    }
                    
                    MessageType messageType;
                    if (recvMsgType(i, messageType) == 0) {
                        // cout << type << endl;
                        if (messageType == REGISTER) {
                            // cout << "Handle REGISTER" << endl;
                            handleRegisterRequest(i);
                        } else if (messageType == LOC_REQUEST) {
                            // cout << "Handle LOCATION" << endl;
                            handleLocRequest(i);
                        } else if (messageType == TERMINATE) {
                            // cout << "Handle TERMINATE" << endl;
                            handleTerminateRequest();
                        } else if (messageType == CACHE_REQUEST) {
                            // cout << "Handle CACHE" << endl;
                            handleCacheRequest(i);
                        }
                    } else {
                        close(i);
                        FD_CLR(i, &master_set);
                    } 
                }
            }
        }
    }

    close(localSocketFd);
    
    return 0;
}

int main(int argc, char *argv[]) {
    int status = runBinder();

    if (status != 0) {
        cout << "BINDER CRASHED" << endl;
    }

    return status;
}