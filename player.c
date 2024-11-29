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
        fprintf(stderr, "[ERR]: Couldn't get address info\n");
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

int createTCPSocket(const char *GSIP, const char *GSPORT, struct addrinfo **res) {
    int tcp_fd, errcode;
    struct addrinfo hints;
    struct timeval timeout;
    timeout.tv_sec = 2;                                             // 2 seconds
    timeout.tv_usec = 0;

    tcp_fd = socket(AF_INET,SOCK_STREAM,0);                         //TCP socket
    if (tcp_fd==-1) {
        fprintf(stderr, "[ERR]: Couldn't create TCP socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;                                        //IPv4
    hints.ai_socktype=SOCK_STREAM;                                  //TCP socket

    errcode=getaddrinfo(GSIP, GSPORT, &hints, res);
    if (errcode != 0) {
        fprintf(stderr, "[ERR]: Couldn't get address info\n");
        close(tcp_fd);
        exit(EXIT_FAILURE);
    }

    return tcp_fd;
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

    //MSG[15] = '\n';
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

    //MSG[11] = '\n';
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

void debugCommand(char input[], int fd, struct addrinfo *res, char player[]) {
       
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[6];
    char PLID [7];
    char TIME [4];
    char MSG[32];
    char buffer[128];  
    char C1[2], C2[2], C3[2], C4[2];    

    sscanf(input, "%s %s %s %s %s %s %s", CMD, PLID, TIME, C1, C2, C3, C4);
    strcpy(player, PLID);

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Player ID is a 6 digit number
    if (strlen(PLID) != 6 || strspn(PLID, "0123456789") != 6) {
        fprintf(stderr, "[ERR]: Invalid PLID: %s\n", PLID);
        return;
    }

    // Validate command format
    if (strlen(C1) != 1 || strlen(C2) != 1 || strlen(C3) != 1 || strlen(C4) != 1) {
        fprintf(stderr, "[ERR]: Invalid code format. Each guess must be a single character.\n");
        return;
    }

    // Play time is a 3 digit number between 1 and 600
    int play_time = atoi(TIME);
    if (play_time < 1 || play_time > 600) {
        fprintf(stderr, "[ERR]: Invalid time: %s\n", TIME);
        return;
    }

    // Prepare the message including the trial number
    snprintf(MSG, sizeof(MSG), "DBG %s %s %s %s %s %s\n", PLID, TIME, C1, C2, C3, C4);

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
}

int main(int argc, char *argv[]) {
    
    char *GSIP = NULL;
    char *GSPORT = NULL;
    
    int OPCODE;
    char line[128];
    char input[128];
    char player[7];
    int trialCount = 1;                                 // Initialize trial counter
    int udp_fd, tcp_fd, errcode;
    struct addrinfo *res_udp, *res_tcp;
    
    parseArguments(argc, argv, &GSIP, &GSPORT);
    //printf("ip: %s\t port: %s\n", GSIP, GSPORT);

    // UDP Socket
    udp_fd = createUDPSocket(GSIP, GSPORT, &res_udp);

    // TCP Socket
    tcp_fd = createTCPSocket(GSIP, GSPORT, &res_tcp);

    // main loop
    while(1) {
        fgets(line, sizeof(line), stdin);               // read the line
        strcpy(input, line);                            // keep the original input
        char *command = strtok(line, " ");              // get the first word
        command[strcspn(command, "\n")] = '\0';
        OPCODE = getCommand(command);

        switch (OPCODE) {
            // start
            case 1:                                     
                startCommand(input, udp_fd, res_udp, player);
                break;
            case 2:
                tryCommand(input, udp_fd, res_udp, player, &trialCount);
                break;
            case 3:
                break;
            case 4:
                break;
            // quit
            case 5:                                     
                quitCommand(input, udp_fd, res_udp, player, 0);
                break;
            // exit   
            case 6:                                     
                quitCommand(input, udp_fd, res_udp, player, 1);
                break;
            // debug
            case 7:
                debugCommand(input, udp_fd, res_udp, player);
                break;
            default:
                fprintf(stderr, "[ERR]: WRONG FORMAT\n");
                break;
        }
    }
}


//setsockopt