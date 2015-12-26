EXEC_NAME=sim
CC=g++

# CFLAGS= -Wall -Wno-write-strings -O0 -g
CFLAGS= -w -Wno-write-strings -O3 -Wall -g
CLIBS= -lpthread

SRC =            		\
	Timer.cpp		\
	TopologyEngine.cpp	\
	DDEngine.cpp		\
	main.cpp

OBJ =  ${SRC:.cpp=.o}

#------------------------------------------------------------

all: ${EXEC_NAME}

${EXEC_NAME}: ${OBJ}
	${CC} ${CFLAGS} -o ${EXEC_NAME} ${OBJ} ${CLIBS}

%.o: %.cpp
	${CC} ${CFLAGS} -c -o $@ $+ ${CLIBS}

clean:
	rm ${EXEC_NAME} *.o *~ *# -rf
