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

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "58015"

void parseArguments(int argc, char *argv[], char **GSIP, char **GSPORT) {
    if (argc == 5) { // Both IP and Port specified
        *GSIP = argv[2];
        *GSPORT = argv[4];
    } else if (argc == 3) {
        if (strcmp(argv[1], "-n") == 0) { // Port omitted
            *GSIP = argv[2];
            *GSPORT = DEFAULT_PORT;
        } else if (strcmp(argv[1], "-p") == 0) { // IP omitted
            *GSIP = DEFAULT_IP;
            *GSPORT = argv[2];
        }
    } else if (argc == 1) { // No arguments specified
        *GSIP = DEFAULT_IP;
        *GSPORT = DEFAULT_PORT;
    } else {
        fprintf(stderr, "[ERR]: INITIAL INPUT HAS WRONG FORMAT\n");
        exit(EXIT_FAILURE);
    }
}

int createUDPSocket(const char *GSIP, const char *GSPORT, struct addrinfo **res) {
    int udp_fd, errcode;
    struct addrinfo hints;
    struct timeval timeout;
    timeout.tv_sec = 2;     // 2 seconds
    timeout.tv_usec = 0;

    // Create UDP socket
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd == -1) {
        fprintf(stderr, "[ERR]: Couldn't create UDP socket\n");
        exit(EXIT_FAILURE);
    }

    // Prepare hints for address resolution
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;         // IPv4
    hints.ai_socktype = SOCK_DGRAM;    // UDP socket

    // Get address info
    errcode = getaddrinfo(GSIP, GSPORT, &hints, res);
    if (errcode != 0) {
        fprintf(stderr, "[ERR]: Couldn't get address info: %s\n", gai_strerror(errcode));
        close(udp_fd);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "[ERR]: setsockopt failed\n");
        close(udp_fd);
        exit(EXIT_FAILURE);
    }

    return udp_fd; // Return the socket file descriptor
}

int getCommand(char* command) {
    if (strcmp(command, "start") == 0)      { return 1;}      // Start new game
    else if (strcmp(command, "try") == 0)   { return 2;}      // try
    else if ((strcmp(command, "show_trials") == 0) || 
            strcmp(command, "st") == 0)     { return 3;}      // show trials
    else if ((strcmp(command, "scoreboard") == 0) ||
            strcmp(command, "sb") == 0)     { return 4;}      // scoreboard
    else if (strcmp(command, "quit") == 0)  { return 5;}      // quit
    else if (strcmp(command, "exit") == 0)  { return 6;}
    else if (strcmp(command, "debug") == 0) { return 7;}      // debug
    else { return 0;}
}

void tryCommand(char input[], int fd, struct addrinfo *res, char PLID[], int *trialCount) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[4];
    char C1[2], C2[2], C3[2], C4[2];
    char MSG[64];
    char buffer[128];

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Parse input to extract the guess
    sscanf(input, "%s %s %s %s %s", CMD, C1, C2, C3, C4);

    // Validate command format
    if (strlen(C1) != 1 || strlen(C2) != 1 || strlen(C3) != 1 || strlen(C4) != 1) {
        fprintf(stderr, "[ERR]: Invalid code format. Each guess must be a single character.\n");
        return;
    }

    // Prepare the message including the trial number
    snprintf(MSG, sizeof(MSG), "TRY %s %s %s %s %s %d\n", PLID, C1, C2, C3, C4, *trialCount);
    
    // printf("%s", MSG);
    // Send the message to the server
    n = sendto(fd, MSG, strlen(MSG), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "[ERR]: Failed to send TRY command.\n");
        return;
    }

    // Increment trial count
    (*trialCount)++;

    // Receive the response
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        fprintf(stderr, "[ERR]: No response from server. Retrying...\n");
        return;
    }

    buffer[n] = '\0'; // Null-terminate the received message

    // handle not recieving response
    while (n < 1) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) /*error*/ exit(1);
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) /*error*/ exit(1);

    }
    write(1,buffer,n);
}

void startCommand(char input[], int fd, struct addrinfo *res, char player[]) {
    
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[6];
    char PLID [7];
    char TIME [4];
    char MSG[16];
    char buffer[128];        

    sscanf(input, "%s %s %s", CMD, PLID, TIME);

    // Register PLID for future context
    strcpy(player, PLID); 

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Player ID is a 6 digit number
    if (strlen(PLID) != 6 || strspn(PLID, "0123456789") != 6) {
        fprintf(stderr, "[ERR]: Invalid PLID: %s\n", PLID);
        return;
    }

    // Play time is a 3 digit number between 1 and 600
    int play_time = atoi(TIME);
    if (play_time < 1 || play_time > 600) {
        fprintf(stderr, "[ERR]: Invalid time: %s\n", TIME);
        return;
    }

    // Message format has to end with \n
    MSG[15] = '\n';
    snprintf(MSG, sizeof(MSG), "SNG %s %s\n", PLID, TIME);

    //printf("%s", MSG);
    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) /*error*/ exit(1);

    // handle not recieving response
    while (n < 1) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) /*error*/ exit(1);
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) /*error*/ exit(1);

    }
    write(1,buffer,n);
}

void quitCommand(char input[], int fd, struct addrinfo *res, char PLID[], int exitCommand) {

    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct timeval timeout;
    timeout.tv_sec = 2;     // 2 seconds
    timeout.tv_usec = 0;
    char CMD[5];
    char MSG[12];
    char buffer[128];        

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    MSG[11] = '\n';
    snprintf(MSG, sizeof(MSG), "QUT %s\n", PLID);
    
    printf("%s", MSG);
    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) /*error*/ exit(1);

    // handle not recieving response
    while (n < 1) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) /*error*/ exit(1);
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) /*error*/ exit(1);
    }

    write(1,buffer,n);

    if (exitCommand) { // in case user asked to exit the app
        freeaddrinfo(res);
        close(fd);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    
    char *GSIP = NULL;
    char *GSPORT = NULL;
    
    int OPCODE;
    char line[128];
    char input[128];
    char player[7];
    int trialCount = 1; // Initialize trial counter
    int udp_fd,errcode;
    struct addrinfo *res;
    
    parseArguments(argc, argv, &GSIP, &GSPORT);
    //printf("ip: %s\t port: %s\n", GSIP, GSPORT);

    // UDP Socket
    udp_fd = createUDPSocket(GSIP, GSPORT, &res);

    // main loop
    while(1) {
        fgets(line, sizeof(line), stdin);               // read the line
        strcpy(input, line);                            // keep the original input
        char *command = strtok(line, " ");              // get the first word
        command[strcspn(command, "\n")] = '\0';
        OPCODE = getCommand(command);

        switch (OPCODE) {
            case 1:                                     // start command
                startCommand(input, udp_fd, res, player);
                break;
            case 2:
                tryCommand(input, udp_fd, res, player, &trialCount);
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:                                     // quit command
                quitCommand(input, udp_fd, res, player, 0);
                break;
            case 6:                                     // exit command
                quitCommand(input, udp_fd, res, player, 1);
                break;
            case 7:
                break;
            default:
                fprintf(stderr, "[ERR]: WRONG FORMAT\n");
                break;
        }
    }
}


//setsockopt