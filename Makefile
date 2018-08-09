CXX = g++
CXXFLAGS = -g -Wall -Wno-unused-label -MMD -DTYPE="${TYPE}"
MAKEFILE_NAME = ${firstword ${MAKEFILE_LIST}}
SOURCES = $(wildcard *.cpp)

OBJECTS1 = binder.o socket.o reasonCode.o
EXEC1 = binder

OBJECTS = ${OBJECTS1}
DEPENDS = ${OBJECTS:.o=.d}
EXECS = ${EXEC1}

.PHONY : all clean

all : ${EXECS}
	${CXX} -c rpc.cpp server.cpp client.cpp server_functions.cpp server_function_skels.cpp
	ar rcs librpc.a reasonCode.o socket.o rpc.o

${EXEC1} : ${OBJECTS1}
	${CXX} $^ -o $@

${OBJECTS} : ${MAKEFILE_NAME}

-include ${DEPENDS}

clean :
	rm -f *.d *.o ${EXECS}
