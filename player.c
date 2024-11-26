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

void startCommand(char input[], int fd, struct addrinfo *res) {
    
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[6];
    char PLID [7];
    char TIME [4];
    char MSG[16];
    char buffer[128];

    sscanf(input, "%s %s %s", CMD, PLID, TIME);
    memset(MSG, 0, sizeof(MSG));

    // Player ID is a 6 digit number
    if (strlen(PLID) != 6 || strspn(PLID, "0123456789") != 6) {
        fprintf(stderr, "[ERR]: Invalid PLID: %s\n", PLID);
        exit(1);
    }

    // Play time is a 3 digit number between 1 and 600
    int play_time = atoi(TIME);
    if (play_time < 1 || play_time > 600) {
        fprintf(stderr, "[ERR]: Invalid time: %s\n", TIME);
        exit(1);
    }

    //printf("%s\n", CMD);
    //printf("%s\n", PLID);
    //printf("%s\n", TIME);

    // Message format has to end with \n
    MSG[15] = '\n'; 

    snprintf(MSG, sizeof(MSG), "SNG %s %s\n", PLID, TIME);

    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) /*error*/ exit(1);
    write(1,buffer,n);
}

int main(int argc, char *argv[]) {
    
    char *GSIP = NULL;
    char *GSPORT = NULL;
    
    int OPCODE;
    char line[128];
    char input[128];

    int udp_fd,errcode;
    struct addrinfo hints,*res;

    if (argc == 5){                                     // every argument was specified
        GSIP = argv[2];
        GSPORT = argv[4];
    } else if (argc == 3) {                             // only one argument was specified
        if (strcmp(argv[1], "-n") == 0) {               // port was ommited
            GSIP = argv[2];
            GSPORT = "58015";                           // 58000 + group number
        } if (strcmp(argv[1], "-p") == 0) {             // ip was ommited
            GSIP = "127.0.0.1";
            GSPORT = argv[2];
        }
    } else if (argc == 1){                              // no argument was specified
        GSIP = "127.0.0.1";
        GSPORT = "58015";
    } else {
        printf("[ERR]: INITIAL INPUT HAS WRONG FORMAT\n");
        exit(0);
    }

    printf("ip: %s\t port: %s\n", GSIP, GSPORT);

    // UDP Socket
    udp_fd=socket(AF_INET,SOCK_DGRAM,0); 
    if(udp_fd==-1) {
        printf("[ERR]: Couldn't create UDP socket\n");
        exit(1);
    }
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;                            //IPv4
    hints.ai_socktype=SOCK_DGRAM;                       //UDP socket
    errcode=getaddrinfo(GSIP, GSPORT, &hints, &res);
    if(errcode!=0) {
        printf("[ERR]: Couldn't get address info\n");
        exit(1);
    }

    // main loop
    while(1) {
        fgets(line, sizeof(line), stdin);               // read the line
        strcpy(input, line);                            // keep the original input
        char *command = strtok(line, " ");              // get the first word
        command[strcspn(command, "\n")] = '\0';
        OPCODE = getCommand(command);

        switch (OPCODE) {
            case 1:                                     // start command
                printf("START NEW GAME\n");
                startCommand(input, udp_fd, res);
                break;
            case 2:
                printf("TRY\n");
                break;
            case 3:
                printf("SHOW TRIALS\n");
                break;
            case 4:
                printf("SCOREBOARD\n");
                break;
            case 5:
                printf("QUIT\n");
                freeaddrinfo(res);
                close(udp_fd);
                exit(1);
            case 6:
                printf("exit\n");
                freeaddrinfo(res);
                close(udp_fd);
                exit(1);
                break;
            case 7:
                printf("DEBUG\n");
                break;
            default:
                printf("[ERR]: WRONG FORMAT\n");
                break;
        }
    }
}


//setsockopt