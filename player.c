#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    
    char *GSIP;
    char *GSPORT;
    
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer[128];

    if (argc == 5){                                     // every argument was specified
        GSIP = argv[2];
        GSPORT = argv[4];
    } else if (argc == 3) {                             // running on the same machine
        if (strcmp(argv[1], "-n") == 0) {               // port was ommited
            GSIP = argv[2];
            GSPORT = "58015";                           // 58000 + group number
        }if ( strcmp(argv[1], "-p") == 0) {             // ip was ommited
            GSIP = "127.0.0.1";
            GSPORT = argv[2];
        }
    }
    
    printf("ip: %s\t port: %s\n", GSIP, GSPORT);

}