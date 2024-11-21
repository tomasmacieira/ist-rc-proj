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
    int size = strlen(command);
    command[size - 1] = '\0';
    if (strcmp(command, "start") == 0) { return 1;}
    else if (strcmp(command, "try") == 0) { return 2;}
    else if (strcmp(command, "show_trials") == 0 || 
            strcmp(command, "st") == 0) { return 3;}
    else if (strcmp(command, "scoreboard") == 0 ||
            strcmp(command, "sb") == 0) { return 4;}
    else if (strcmp(command, "quit") == 0) { return 5;}
    else if (strcmp(command, "exit") == 0) { return 6;}
    else if (strcmp(command, "debug") == 0) { return 7;}
    else { return 0;}
}

void startCommand(int fd, struct addrinfo *res) {

}

int main(int argc, char *argv[]) {
    
    char *GSIP;
    char *GSPORT;
    
    fd_set inputs, out_fds;
    struct timeval timeout;

    int errcode, OPCODE;
    ssize_t n;

    // UDP socket info
    struct addrinfo hints,*res;
    struct sockaddr_in udp_addr;
    socklen_t addrlen;
    int udp_fd;

    char buffer[128];
    char line[128];
    char prt_str[90];

    if (argc == 5){                                     // every argument was specified
        GSIP = argv[2];
        GSPORT = argv[4];
    } else if (argc == 3) {                             // only one argument was specified
        if (strcmp(argv[1], "-n") == 0) {               // port was ommited
            GSIP = argv[2];
            GSPORT = "58015";                           // 58000 + group number
        } if ( strcmp(argv[1], "-p") == 0) {            // ip was ommited
            GSIP = "127.0.0.1";
            GSPORT = argv[2];
        } else {                                        // no argument was specified
            GSIP = "127.0.0.1";
            GSPORT = "58015";
        }
    }

    /*memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

    if((errcode=getaddrinfo(GSIP,GSPORT,&hints,&res))!=0)
        exit(1);// On error

    udp_fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(udp_fd==-1) exit(1);

    if(bind(udp_fd,res->ai_addr,res->ai_addrlen)==-1) {
        sprintf(prt_str,"Bind error UDP server\n");
        write(1,prt_str,strlen(prt_str));
        exit(1);// On error
    }
    if(res!=NULL) freeaddrinfo(res);

    FD_ZERO(&inputs);                                   // Clear input mask
    FD_SET(0,&inputs);                                  // Set standard input channel on
    FD_SET(udp_fd,&inputs);                             // Set UDP channel on
    */
    // main loop
    while(1) {
        fgets(line, sizeof(line), stdin);               // read the line
        char *command = strtok(line, "");               // get the first word
        OPCODE = getCommand(command);

        switch (OPCODE) {
            case 1:                                     // start command
                printf("ola\n");
                //startCommand(fd, res);
                break;
            case 2:
                break;
            case 5:
                exit(1);
            default:
                break;
        }
    }

    printf("ip: %s\t port: %s\n", GSIP, GSPORT);

}