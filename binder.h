#include "socket.h"
#include "reasonCode.h"
#include "rpc.h"

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <cstdlib>
#include <signal.h>
#include <map>
#include <strings.h>
#include <queue>

using namespace std;

struct serverIdentifier {
    string server_identifier;
    unsigned short port;

    serverIdentifier(string server_identifier, unsigned short port) : server_identifier(server_identifier), port(port) {}
};

bool operator == (const serverIdentifier &l, const serverIdentifier &r);

struct functionIdentifier {
    string name;
    int *argTypes;

    functionIdentifier(string name, int *argTypes) : name(name), argTypes(argTypes){}
};

bool operator < (const functionIdentifier &l, const functionIdentifier &r);

int runBinder();
