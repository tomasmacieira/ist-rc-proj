#ifndef GS_H
#define GS_H

#define DEFAULT_PORT "58015"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <sys/select.h>

void parseArguments(int argc, char *argv[], int *verbose, char **GSPORT);

int createUDPSocket(const char *GSPORT, struct addrinfo **res);

int createTCPSocket(const char *GSPORT, struct addrinfo **res);

#endif