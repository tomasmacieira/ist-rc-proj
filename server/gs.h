#ifndef GS_H
#define GS_H

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <time.h>
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

#define DEFAULT_PORT "58015"
#define DEFAULT_PLAYER "123456"
#define MAX_TRIES 8

#define R 1
#define G 2
#define B 3
#define Y 4
#define O 5
#define P 6

typedef struct player {
    time_t startTime;
    time_t maxTime;
    char PLID[7];
    int fd;             // game fd
    char code[5];
    int attempts;
    char tries[MAX_TRIES][5];
} player_t;


void parseArguments(int argc, char *argv[], int *verbose, char **GSPORT);

int createUDPSocket(const char *GSPORT, struct addrinfo **res);

int createTCPSocket(const char *GSPORT, struct addrinfo **res);

void handleUDPrequest(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose);

int parseCommand(char command[]);

void startCommand(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t , int verbose);

void tryCommand(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose);

void chooseCode(int colorCode[], struct player *p);

void printDescription(char input[], char PLID[], struct sockaddr *client_addr, socklen_t);

void createGameFile(struct player *p, char mode, int timeLimit);

void writeTry(struct player *, int nB, int nW);

int checkPreviousTries(struct player *p, char try[]);

int checkPreviousTry(struct player *p, char try[]);

void quitCommand(char input[], int fd, struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose);

int validPLID(char PLID[]);

void endGame(player_t *player);

void debugCommand(char input[], int fd, struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose);

int validTime(char time[]);

#endif