#include "player.h"

int main(int argc, char *argv[]) {
    
    char *GSIP = NULL;
    char *GSPORT = NULL;
    
    int OPCODE;
    char line[128];
    char input[128];
    char player[] = DEFAULT_PLAYER;
    int trialCount = 1;                                   // Initialize trial counter
    int udp_fd, tcp_fd, errcode;
    struct addrinfo *res_udp, *res_tcp;
    
    parseArguments(argc, argv, &GSIP, &GSPORT);
    printf("ip: %s\t port: %s\n", GSIP, GSPORT);

    // UDP Socket
    udp_fd = createUDPSocket(GSIP, GSPORT, &res_udp);

    // main loop
    while(1) {
        fgets(line, sizeof(line), stdin);                 // read the line
        strcpy(input, line);                              // keep the original input
        char *command = strtok(line, " ");                // get the first word
        command[strcspn(command, "\n")] = '\0';
        OPCODE = getCommand(command);

        switch (OPCODE) {
            // start
            case 1:                                     
                startCommand(input, udp_fd, res_udp, player, &trialCount);
                break;
            // try
            case 2:
                tryCommand(input, udp_fd, res_udp, player, &trialCount);
                break;
            // show trials
            case 3:
                tcp_fd = createTCPSocket(GSIP, GSPORT, &res_tcp);
                showtrialsCommand(tcp_fd, res_tcp, player);
                break;
            /// scoreboard
            case 4:
                tcp_fd = createTCPSocket(GSIP, GSPORT, &res_tcp); 
                scoreboardCommand(tcp_fd, res_tcp);
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
                debugCommand(input, udp_fd, res_udp, player, &trialCount);
                break;
            default:
                fprintf(stderr, "[ERR]: WRONG FORMAT\n");
                break;
        }
    }
}

void parseArguments(int argc, char *argv[], char **GSIP, char **GSPORT) {
    if (argc == 5) {                             // Both IP and Port specified
        *GSIP = argv[2];
        *GSPORT = argv[4];
    } else if (argc == 3) {
        if (strcmp(argv[1], "-n") == 0) {        // Port omitted
            *GSIP = argv[2];
            *GSPORT = DEFAULT_PORT;
        } else if (strcmp(argv[1], "-p") == 0) { // IP omitted
            *GSIP = DEFAULT_IP;
            *GSPORT = argv[2];
        }
    } else if (argc == 1) {                      // No arguments specified
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
    timeout.tv_sec = 2;                 // 2 seconds
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

    return udp_fd;                      // Return the socket descriptor
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
    int attempts = 0;

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Parse input to extract the guess
    sscanf(input, "%s %s %s %s %s", CMD, C1, C2, C3, C4);

    // Validate command format
    if (strlen(C1) != 1 || strlen(C2) != 1 || strlen(C3) != 1 || strlen(C4) != 1) {
        fprintf(stderr, "[ERR]: Invalid code format. Each guess must be a single character: TRY <C1> <C2> <C3> <C4>.\n");
        return;
    }

    // Prepare the message including the trial number
    snprintf(MSG, sizeof(MSG), "TRY %s %s %s %s %s %d\n", PLID, C1, C2, C3, C4, *trialCount);
    
    // Send the message to the server
    n = sendto(fd, MSG, strlen(MSG), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "[ERR]: Failed to send TRY command.\n");
        return;
    }

    // Receive the response
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        fprintf(stderr, "[ERR]: No response from server. Retrying...\n");
        return;
    }
    buffer[n] = '\0';

    // handle not recieving response
    while (n < 0 && errno == EWOULDBLOCK || errno == EAGAIN && attempts < MAX_ATTEMPTS) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
            exit(EXIT_FAILURE);
        }
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
            exit(EXIT_FAILURE);
        }
        attempts++;

    }
    // Check the message content before incrementing the trial count (Acrescentar ERR)
    if (strncmp(buffer, "RTR DUP", 7) != 0 && strncmp(buffer, "RTR ERR", 7) != 0) {
        (*trialCount)++;
    }
    analyseResponse(buffer);
}

void startCommand(char input[], int fd, struct addrinfo *res, char player[], int *trialCount) {
    
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[6];
    char PLID [7];
    char TIME [4];
    char MSG[16];
    char buffer[128]; 
    int attempts = 0;       

    sscanf(input, "%s %s %s", CMD, PLID, TIME);
    *trialCount = 1;
    // Register PLID for future context
    strcpy(player, PLID); 

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Player ID is a 6 digit number
    if (strlen(PLID) != 6 || strspn(PLID, "0123456789") != 6) {
        fprintf(stderr, "[ERR]: Invalid PLID: %s\nPLID must be a 6 digit number\n", PLID);
        return;
    }

    // Play time is a 3 digit number between 1 and 600
    int play_time = atoi(TIME);
    if (play_time < 1 || play_time > 600) {
        fprintf(stderr, "[ERR]: Invalid time: %s\nTime must be a 3 digit number between 1 and 600\n", TIME);
        return;
    }

    snprintf(MSG, sizeof(MSG), "SNG %s %s\n", PLID, TIME);

    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
        exit(EXIT_FAILURE);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
        printf("Client expected response from: %s:%d\n",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));
        exit(EXIT_FAILURE);
    }

    // handle not recieving response
    while (n < 0 && errno == EWOULDBLOCK || errno == EAGAIN && attempts < MAX_ATTEMPTS) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
            exit(EXIT_FAILURE);
        }
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
            exit(EXIT_FAILURE);
        }
        attempts++;

    }
    analyseResponse(buffer);
}

void quitCommand(char input[], int fd, struct addrinfo *res, char player[], int exitCommand) {

    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct timeval timeout;
    char CMD[5];
    char MSG[12];
    char buffer[128];   
    int attempts = 0;     

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    snprintf(MSG, sizeof(MSG), "QUT %s\n", player);
    MSG[sizeof(MSG) - 1] = '\0';
    
    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
        exit(EXIT_FAILURE);
    }

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
        exit(EXIT_FAILURE);
    }

    // handle not recieving response
    while (n < 0 && errno == EWOULDBLOCK || errno == EAGAIN && attempts < MAX_ATTEMPTS) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
            exit(EXIT_FAILURE);
        }
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
            exit(EXIT_FAILURE);
        }
        attempts++;
    }
    
    // in case user asked to exit the app
    if (exitCommand) { 
        fprintf(stdout,"Exiting...\n");
        freeaddrinfo(res);
        close(fd);
        exit(0);
    }

    analyseResponse(buffer);
}

void debugCommand(char input[], int fd, struct addrinfo *res, char player[], int *trialCount) {
       
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char CMD[6];
    char PLID [7];
    char TIME [4];
    char MSG[32];
    char buffer[128];  
    char C1[2], C2[2], C3[2], C4[2]; 
    int attempts = 0;   

    sscanf(input, "%s %s %s %s %s %s %s", CMD, PLID, TIME, C1, C2, C3, C4);
    strcpy(player, PLID);

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));

    // Player ID is a 6 digit number
    if (strlen(PLID) != 6 || strspn(PLID, "0123456789") != 6) {
        fprintf(stderr, "[ERR]: Invalid PLID: %s\nMust be a 6 digit number\n", PLID);
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
        fprintf(stderr, "[ERR]: Invalid time: %s\nMust be a 3 digit number between 1 and 600\n", TIME);
        return;
    }

    // Prepare the message including the trial number
    snprintf(MSG, sizeof(MSG), "DBG %s %s %s %s %s %s\n", PLID, TIME, C1, C2, C3, C4);
    *trialCount = 1;

    n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
        exit(EXIT_FAILURE);
    }

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
    if(n==-1) {
        fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
        exit(EXIT_FAILURE);
    }

    // handle not recieving response
    while (n < 0 && errno == EWOULDBLOCK || errno == EAGAIN && attempts < MAX_ATTEMPTS) {
        n=sendto(fd,MSG,strlen(MSG),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't send UDP request\n");
            exit(EXIT_FAILURE);
        }
        n=recvfrom(fd,buffer,128,0, (struct sockaddr*)&addr, &addrlen);
        if(n==-1) {
            fprintf(stderr, "[ERR]: Couldn't recieve UDP response\n");
            exit(EXIT_FAILURE);
        }
        attempts++;
    }

    analyseResponse(buffer);
}

void scoreboardCommand(int fd, struct addrinfo *res) {
    ssize_t n, bytesRead;
    int i = 0, fptr;
    char ch;
    char MSG[] = "SSB\n";  
    char buffer[1024], status[100], Fname[50];     

    memset(buffer, 0, sizeof(buffer));
    memset(status, 0, sizeof(status));
    memset(Fname, 0, sizeof(Fname));

    // Connect to the server
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[ERR]: Failed to establish TCP connection with GS.\n");
        return;
    }

    // Send the scoreboard request
    n = write(fd, MSG, strlen(MSG));
    if (n == -1) {
        fprintf(stderr, "[ERR]: Failed to send SCOREBOARD request.\n");
        close(fd);
        return;
    }

    // Read status
    // Read format: SSB status [Fname Fsize Fdata]
    while ((n = read(fd, &ch, 1)) > 0) {            // Read one character at a time
        if (i < sizeof(status)) {
            status[i++] = ch;
            bytesRead++;
        }
        if (ch == '\n') {                           // End of the first line
            status[i++] = ch;
            break;
        }
    }
    if (n <= 0) {
        fprintf(stderr, "[ERR]: Failed to read status.\n");
        close(fd);
        return;
    }

    analyseResponse(status);

    if (strcmp(status, "SSB EMPTY") == 0) {
        close(fd);
        return;}

    // Get the filename and filesize
    sscanf(status, "%*s %*s %s\n", Fname);

    bytesRead = read(fd, buffer, sizeof(buffer));


    // Save the file
    fptr = open(Fname, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fptr < 0) {
        fprintf(stderr, "[ERR]: Failed to create file %s.\n", Fname);
        close(fptr);
        close(fd);
        return;
    }
    write(fptr, buffer, bytesRead);
    close(fd);
    close(fptr);
}

void showtrialsCommand(int fd, struct addrinfo *res, char player[]) {
    int fptr, i = 0;
    char MSG[12], buffer[1024], status[100], Fname[32], RSP[5], ch;
    ssize_t Fsize, bytesRead = 0, totalBytes = 0, n;

    memset(MSG, 0, sizeof(MSG));
    memset(buffer, 0, sizeof(buffer));
    memset(status, 0, sizeof(status));
    memset(Fname, 0, sizeof(Fname));

    snprintf(MSG, sizeof(MSG), "STR %s\n", player);

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[ERR]: Failed to establish TCP connection with GS.\n");
        return;
    }

    n = write(fd, MSG, strlen(MSG));
    if (n == -1) {
        fprintf(stderr, "[ERR]: Failed to send SHOW TRIALS request.\n");
        close(fd);
        return;
    }

    // Read status
    // Read format: RST status [Fname Fsize Fdata]
    while ((n = read(fd, &ch, 1)) > 0) {            // Read one character at a time
        if (i < sizeof(status)) {
            status[i++] = ch;
            bytesRead++;
        }
        if (ch == '\n') {                           // End of the first line
            status[i++] = ch;
            break;
        }
    }
    if (n <= 0) {
        fprintf(stderr, "[ERR]: Failed to read status.\n");
        close(fd);
        return;
    }

    analyseResponse(status);

    // Get the filename and filesize
    sscanf(status, "%*s %s %s\n", RSP, Fname);

    // No active or inactive games for this player
    if (strcmp(RSP, "NOK") == 0) {
        close(fd);
        return;
    }

    bytesRead = read(fd, buffer, sizeof(buffer));

    // Save the file
    fptr = open(Fname, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fptr < 0) {
        fprintf(stderr, "[ERR]: Failed to create file %s.\n", Fname);
        close(fptr);
        close(fd);
        return;
    }
    write(fptr, buffer, bytesRead);
    close(fd);
    close(fptr);
}

void analyseResponse(char response[]) {
    char nT[2], nB[2], nW[2];
    char C1[2], C2[2], C3[2], C4[2];
    char code[8];
    char Fname[50], Fsize[10];

    // Handle start request status 
    if (strcmp(response, "RSG NOK\n") == 0) {
        fprintf(stdout, "Current player already has an ongoing game\n");
        return;
    }
    if (strcmp(response, "RSG OK\n") == 0) {
        fprintf(stdout, "Game started! You have 8 tries and the following plays: R,G,B,Y,O and P\n");
        return;
    }
    if (strcmp(response, "RSG ERR\n") == 0) {
        fprintf(stdout, "Invalid syntax\nFORMAT: start PLID TIME\n");
        return;
    }

    // Handle try request status
    if (strncmp(response, "RTR OK ", 7) == 0) {
        sscanf(response, "RTR OK %s %s %s\n", nT, nB, nW);
        
        // Player won
        if (strcmp(nB, "4") == 0) {
            fprintf(stdout, "Game won, you guessed the right code!\n");
            return;
        }

        fprintf(stdout, 
                "####### Try number %s #####\n"
                "Number of correct colors in the right position: %s\n"
                "Number of correct colors in the wrong position: %s\n",
                nT, nB, nW);
        return;
    }
    if (strncmp(response, "RTR DUP", 7) == 0) {
        fprintf(stdout, "Duplicate try!\n");
        return;
    }
    if (strncmp(response, "RTR INV", 7) == 0) {
        fprintf(stdout, "Trial number not expected\n");
        return;
    } 
    if (strncmp(response, "RTR NOK", 7) == 0) {
        fprintf(stdout, "Current player does not have an active game!\n");
        return;
    }
    if (strncmp(response, "RTR ENT", 7) == 0) {
        sscanf(response, "RTR ENT %s %s %s %s\n", C1, C2, C3, C4);
        fprintf(stdout, "No more attempts left! Secret color code was: %s %s %s %s\n", C1, C2, C3, C4);
        return;
    } 
    if (strncmp(response, "RTR ETM", 7) == 0) {
        // Extract the secret key from the response
        sscanf(response, "RTR ETM %s %s %s %s\n", C1, C2, C3, C4);
        fprintf(stdout, "No more time left to play! Secret color code was: %s %s %s %s\n", C1, C2, C3, C4);
        return;
    }
    if (strncmp(response, "RTR ERR", 7) == 0) {
        fprintf(stdout, "Wrong Syntax on try request\nFORMAT: TRY <C1> <C2> <C3> <C4>\nAvailable colors: R G B Y O P\n");
        return;
    }

    // handle quit and exit responses
    if (strncmp(response, "RQT OK ", 7) == 0) {
        sscanf(response, "RQT OK %s %s %s %s\n", C1, C2, C3, C4);
        fprintf(stdout, "Quiting game... Secret color code was: %s %s %s %s\n", C1, C2, C3, C4);
        return;
    }
    if (strncmp(response, "RQT NOK", 7) == 0) {
        fprintf(stdout, "Current player does not have an active game\n");
        return;
    }
    if (strncmp(response, "RQT ERR", 7) == 0) {
        fprintf(stdout, "Error\n");
        return;
    }


    // handle debug responses
    if (strcmp(response, "RDB NOK\n") == 0) {
        fprintf(stdout, "Current player has an active game\n");
        return;
    }
    if (strcmp(response, "RDB OK\n") == 0) {
        fprintf(stdout, "Debugging started... Using the secret color code provided\n");
        return;
    }
    if (strcmp(response, "RDB ERR\n") == 0) {
        fprintf(stdout, "Wrong Syntax on debug request\nFORMAT: debug <PLID> <max_playtime> <C1> <C2> <C3> <C4>\nAvailable colors: R G B Y O P\n");
        return;
    }

    // handle show trials responses
    if (strncmp(response, "RST ACT", 7) == 0) {
        sscanf(response, "RST ACT %s %s", Fname, Fsize);
        fprintf(stdout, "Downloading file %s with %s bytes, containing the most recent attempts\n", Fname, Fsize);
        return;
    }
    if (strncmp(response, "RST FIN", 7) == 0) {
        fprintf(stdout, "Current player does not have an active game\n");
        sscanf(response, "RST FIN %s %s", Fname, Fsize);
        fprintf(stdout, "Downloading file %s with %s bytes, containing the summary of the most recent game\n", Fname, Fsize);
        return;
    }
    if (strncmp(response, "RST NOK", 7) == 0) {
        fprintf(stdout, "Current player does not have any active or finished game\n");
        return;
    }

    // handle scoreboard responses
    if (strncmp(response, "RSS OK", 6) == 0) {
        sscanf(response, "RSS OK %s %s", Fname, Fsize);
        fprintf(stdout, "Downloading file %s with %s bytes, containing the scoreboard\n", Fname, Fsize);
        return;
    }
    if (strncmp(response, "RSS EMPTY", 9) == 0) {
        fprintf(stdout, "No game was yet won by any player\n");
        return;
    }
}