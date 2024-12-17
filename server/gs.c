#include "gs.h"

int main(int argc, char *argv[]) {

    int verbose = 0;
    char *GSPORT = NULL;
    int udp_fd, tcp_fd, client_fd;
    struct addrinfo *res_udp, *res_tcp;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    char input[128];

    int colorCode[4];

    parseArguments(argc, argv, &verbose, &GSPORT);

    player_t p;
    strcpy(p.PLID, DEFAULT_PLAYER);
    player_t *player = &p;
    player->attempts = 0;
    player->gameStatus = 0;

    printf("PORT: %s VERBOSE: %d\n", GSPORT, verbose);

    udp_fd = createUDPSocket(GSPORT, &res_udp);
    tcp_fd = createTCPSocket(GSPORT, &res_tcp);

    fd_set inputs, testfds;
    FD_ZERO(&inputs);
    FD_SET(udp_fd, &inputs);
    FD_SET(tcp_fd, &inputs);

    int max_fd = (udp_fd > tcp_fd) ? udp_fd : tcp_fd;

    while (1) {
        struct timeval timeout;
        timeout.tv_sec = 10;            // Timeout interval: 10 seconds
        timeout.tv_usec = 0;

        testfds = inputs;               // Copy inputs to testfds

        int out_fds = select(max_fd + 1, &testfds, NULL, NULL, &timeout);

        if (out_fds < 0) {
            fprintf(stderr, "[ERR]: Select function failed\n");
            break;
        } else if (out_fds == 0) {
            fprintf(stderr, "Timeout occurred, no activity in 10 seconds.\n");
            continue; 
        }

        // Check UDP socket
        if (FD_ISSET(udp_fd, &testfds)) {

            int recv_bytes = recvfrom(udp_fd, buffer, sizeof(buffer), 0,
                                      (struct sockaddr *)&client_addr, &client_len);
            if (recv_bytes < 0) {
                fprintf(stderr, "[ERR]: Error receiving UDP message\n");
                continue;
            }

            handleUDPrequest(buffer, udp_fd, colorCode, player, (struct sockaddr *)&client_addr, client_len, verbose);
        }

        // Check TCP socket
        if (FD_ISSET(tcp_fd, &testfds)) {

            client_fd = accept(tcp_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("Error accepting TCP connection");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("Error forking for TCP connection");
                close(client_fd);
            } else if (pid == 0) {
                // Child process: Handle TCP connection
                // HANDLE TCP
            close(client_fd); // Parent doesn't need the client socket
            }
        }

    }
    // Cleanup
    close(udp_fd);
    close(tcp_fd);
    freeaddrinfo(res_udp);
    freeaddrinfo(res_tcp);

    return 0;
}

void parseArguments(int argc, char *argv[], int *verbose, char **GSPORT) {
    if (argc == 4) {        // Port and verbose specified
        *verbose = 1;
        *GSPORT = argv[2];
    } else if (argc == 3) { // Only port specified
        *GSPORT = argv[2];
        *verbose = 0;
    } else if (argc == 2) { // Port not specified, verbose on
        *GSPORT = DEFAULT_PORT;
        *verbose = 1;
    } else if (argc == 1) { // Nothing specified
        *GSPORT = DEFAULT_PORT;
        *verbose = 0;
    } else {
        fprintf(stderr, "[ERR]: INITIAL INPUT HAS WRONG FORMAT\n");
        exit(EXIT_FAILURE);
    }
}

int createUDPSocket(const char *GSPORT, struct addrinfo **res) {
    int errcode;
    struct addrinfo hints;
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        fprintf(stderr, "UDP socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;            // IPv4
    hints.ai_socktype=SOCK_DGRAM;       // UDP socket
    hints.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL, GSPORT, &hints, res);
    if(errcode!=0) {
        fprintf(stderr, "Couldnt read address\n");
        exit(EXIT_FAILURE);
    }

    if (bind(udp_fd, (*res)->ai_addr, (*res)->ai_addrlen) < 0) {
        perror("UDP socket bind failed");
        exit(EXIT_FAILURE);
    }
    return udp_fd;
}

int createTCPSocket(const char *GSPORT, struct addrinfo **res) {
    
    int errcode;
    struct addrinfo hints;
    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        fprintf(stderr, "TCP socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;            // IPv4
    hints.ai_socktype=SOCK_STREAM;      // TCP socket
    hints.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL, GSPORT, &hints, res);
    if(errcode!=0) {
        fprintf(stderr, "Couldnt read address\n");
        exit(EXIT_FAILURE);
    }

    if (bind(tcp_fd, (*res)->ai_addr, (*res)->ai_addrlen) < 0) {
        perror("TCP socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_fd, 5) < 0) {
        fprintf(stderr, "TCP listen failed\n");
        exit(EXIT_FAILURE);
    }

    return tcp_fd;
}

void handleUDPrequest(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose) {

    int OPCODE;
    char CMD[4];
    
    printf("input: %s", input);
    sscanf(input, "%s %*s\n", CMD);
    OPCODE = parseCommand(CMD);
    switch (OPCODE) {
    // start
    case 1:
        startCommand(input, fd, colorCode, p, client_addr, client_len, verbose);
        break;
    // try
    case 2:
        tryCommand(input, fd, colorCode, p, client_addr, client_len, verbose);
        break;
    // show trials
    case 3:
        break;
    // scoreboard
    case 4:
        break;
    // quit or exit
    case 5:
        quitCommand(input, fd, p, client_addr, client_len, verbose);
        break;
    // debug
    case 6:
        debugCommand(input, fd, p, client_addr, client_len, verbose);
        break;
    default:
        break;
    }

}

int parseCommand(char command[]) {
    if (strcmp(command, "SNG") == 0)        { return 1;}      // Start new game
    else if (strcmp(command, "TRY") == 0)   { return 2;}      // try
    else if (strcmp(command, "STR") == 0)   { return 3;}      // show trials
    else if (strcmp(command, "SSB") == 0)   { return 4;}      // scoreboard
    else if (strcmp(command, "QUT") == 0)   { return 5;}      // quit or exit
    else if (strcmp(command, "DBG") == 0)   { return 6;}      // debug
    else { return 0;}
}

void startCommand(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    char PLID[7];
    char gameTime[4];
    char response[100];
    char code[5];

    sscanf(input, "SNG %s %s\n", PLID, gameTime);

    if (strlen(PLID) != 6 || strlen(gameTime) != 3 || (atoi(gameTime) < 1 || atoi(gameTime) > 600)) {
        snprintf(response, sizeof(response), "RSG ERR\n");
    } if (strcmp(PLID, p->PLID) == 0) {
        snprintf(response, sizeof(response), "RSG NOK\n");
    } else {
        chooseCode(colorCode, p);
        strcpy(p->PLID, PLID);
        p->attempts = 0;
        // Record the start time and max play time
        p->maxTime = atoi(gameTime);            // Max play time from input
        p->gameStatus = 1;
        snprintf(response, sizeof(response), "RSG OK\n");
        createGameFile(p, 'P', atoi(gameTime));
    }

    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr,"[ERR]: Couldn't send UDP response");
        exit(EXIT_FAILURE);
    }

}

int checkColors(char C1, char C2, char C3, char C4) {
    char validColors[] = "RGBYOP"; // Valid color codes
    // Check each input against the valid colors
    if (!strchr(validColors, C1) || !strchr(validColors, C2) || 
        !strchr(validColors, C3) || !strchr(validColors, C4)) {
        return 1; // Invalid color found
    }
    return 0; // All colors are valid
}

int checkKey(struct player *p, char* C1, char* C2, char* C3, char* C4) {
    if(p->code[0] == C1[0] && p->code[1] == C2[0] && p->code[2] == C3[0] && p->code[3] == C4[0]){
        return 0;
    }
    return 1;
}

void tryCommand(char input[], int fd, int colorCode[], struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char C1[2], C2[2], C3[2], C4[2];
    char PLID[7];
    char response[100];
    char try[5];
    int attempt;
    int nB = 0, nW = 0;
    time_t currentTime = time(NULL);
    p->attempts++;
    sscanf(input, "TRY %s %s %s %s %s %d\n", PLID, C1, C2, C3, C4, &attempt);
    memset(try, 0, sizeof(try));
     // register try
    strcpy(try, C1);
    strcat(try, C2);
    strcat(try, C3);
    strcat(try, C4);

    if(strcmp(p->PLID,DEFAULT_PLAYER)==0 || strcmp(p->PLID, PLID)!= 0){
        snprintf(response, sizeof(response), "RTR NOK\n");
    }
    else if (checkColors(C1[0], C2[0], C3[0], C4[0]) || strlen(p->PLID) != 6) {
        snprintf(response, sizeof(response), "RTR ERR\n");
    }
    else if(difftime(currentTime, p->startTime) > p->maxTime) {
        snprintf(response, sizeof(response), "RTR ETM %c %c %c %c\n", p->code[0], p->code[1], p->code[2], p->code[3]);
        endGame(p);
    }

    else if(p->attempts != attempt && !(p->attempts - 1 == attempt && checkPreviousTry(p, try))){
        snprintf(response, sizeof(response), "RTR INV\n");
    }

    else if (p->attempts >= 8) {
        if(checkKey(p, C1, C2, C3, C4)){
            snprintf(response, sizeof(response), "RTR ENT\n");
        }
        endGame(p);
    }
    else if (checkPreviousTries(p, try)) {
        snprintf(response, sizeof(response), "RTR DUP\n");
        p->attempts--;
    }
    else {
        printf("correct code is: %s\n", p->code);
        printf("played code was %s\n", try);
        for (int i = 0; i < 4; i++) {
            int temp_nB = 0, temp_nW = 0;
            if(C1[0] == p->code[i]){
                if(i == 0){
                    temp_nB++;
                }
                else {
                    temp_nW++;
                }
            }
            if(C2[0] == p->code[i]){
                if(i == 1){
                    temp_nB++;
                    temp_nW = 0;
                }
                else {
                    temp_nW++;
                }
            }
            if(C3[0] == p->code[i]){
                if(i == 2){
                    temp_nB++;
                    temp_nW = 0;
                }
                else {
                    temp_nW++;
                }
            }
            if(C4[0] == p->code[i]){
                if(i == 3){
                    temp_nB++;
                    temp_nW = 0;
                }
                else{
                    temp_nW++;
                }
            }
            if(temp_nB != 0){
                nB++;
            }
            else if(temp_nW != 0){
                nW++;
            }
        }
        snprintf(response, sizeof(response), "RTR OK %d %d %d\n", p->attempts, nB, nW);
        strcpy(p->tries[p->attempts - 1], try);
        writeTry(p, nB, nW);
        if(nB == 4){
            endGame(p);
        }
    }


    if (verbose) {
        printDescription(input, p->PLID, client_addr, client_len);
    }
    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr,"[ERR]: Couldn't send UDP response");
        exit(EXIT_FAILURE);
    }
}

void createGameFile(struct player *p, char mode, int timeLimit) {
    char filepath[48];
    char dirname[48];
    char date[20]; 
    char hours[20];
    char firstLine[128];
    time_t fulltime;
    struct tm *current_time;

    time(&fulltime);
    p->startTime = fulltime;

    current_time = gmtime(&fulltime);

    snprintf(date, sizeof(date), "%4d-%02d-%02d",
             current_time->tm_year + 1900,
             current_time->tm_mon + 1,
             current_time->tm_mday);

    snprintf(hours, sizeof(hours), "%02d:%02d:%02d",
             current_time->tm_hour,
             current_time->tm_min,
             current_time->tm_sec);


    snprintf(dirname, sizeof(dirname), "./server/games/%s", p->PLID);
    mkdir(dirname, 0755);

    snprintf(filepath, sizeof(filepath), "./server/games/%s/GAME_%s.txt", p->PLID, p->PLID);

    p->fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (p->fd == -1) {
        fprintf(stderr, "[ERR]: open failed\n");
        exit(EXIT_FAILURE);
    }

    snprintf(firstLine, sizeof(firstLine),
             "%-6s %c %s %d %s %s %ld\n",
             p->PLID,           // PLID
             mode,              // mode (Play ou Debug)
             p->code,           // secret code to win 
             timeLimit,         // time to win
             date,              // YYYY-MM-DD
             hours,             // HH:MM:SS
             fulltime);         // time when game started

    if (write(p->fd, firstLine, strlen(firstLine)) == -1) {
        fprintf(stderr, "[ERR]: write failed\n");
        close(p->fd);
        exit(EXIT_FAILURE);
    }
}

void chooseCode(int code[], struct player *p) { 
    srand(time(NULL));

    for (int i = 0; i < 4; i++) {
        code[i] = (rand() % 6) + 1;  // Generate a number between 1 and 6

        switch (code[i]) {
            case R:
                p->code[i] = 'R';
                break;
            case G:
                p->code[i] = 'G';
                break;
            case B:
                p->code[i] = 'B';
                break;
            case Y:
                p->code[i] = 'Y';
                break;
            case O:
                p->code[i] = 'O';
                break;
            case P:
                p->code[i] = 'P';
                break;
        }
    }

    p->code[4] = '\0';

}

void printDescription(char input[], char PLID[], struct sockaddr *client_addr, socklen_t client_len) {
    char client_ip[INET_ADDRSTRLEN];

    struct sockaddr_in *addr_in = (struct sockaddr_in *)client_addr;

    // Get the IP address
    inet_ntop(AF_INET, &addr_in->sin_addr, client_ip, sizeof(client_ip));

    // Get the port number
    int client_port = ntohs(addr_in->sin_port);

    char CMD[4];
    sscanf(input, "%s %*s", CMD);

    printf("Received UDP request:\n");
    printf(" - PLID: %s\n", PLID);
    printf(" - Request Type: %s\n", CMD);
    printf(" - From IP: %s, Port: %d\n", client_ip, client_port);
}

void writeTry(struct player *p, int nB, int nW) {
    char line[64];

    snprintf(line, sizeof(line), "T: %s %d %d %ld\n", p->tries[p->attempts - 1], nB, nW, p->startTime);

    if (write(p->fd, line, strlen(line)) == -1) {
        fprintf(stderr, "[ERR]: write failed\n");
        close(p->fd);
        exit(EXIT_FAILURE);
    }
}

int checkPreviousTries(struct player *p, char try[]) {
    for (int i = 0; i < MAX_TRIES; i++) {
        if (strcmp(p->tries[i], try) == 0) return 1;
    }
    return 0;
}

int checkPreviousTry(struct player *p, char try[]) {
    if (strcmp(p->tries[p->attempts - 2], try) == 0) return 1;
    return 0;
}

void quitCommand(char input[], int fd, struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    
    char PLID[7];
    char response[100];
    char code[8];
    char C1[2]; char C2[2]; char C3[2]; char C4[2];

    sscanf(input, "QUT %s\n", PLID);

    // check if PLID had ongoing game
    if (!p->gameStatus && validPLID(PLID)) { snprintf(response, sizeof(response), "RQT NOK\n");}
    else if (p->gameStatus && validPLID(PLID)) { 

        C1[0] = p->code[0];
        C1[1] = '\0';  

        C2[0] = p->code[1];
        C2[1] = '\0';

        C3[0] = p->code[2];
        C3[1] = '\0';

        C4[0] = p->code[3];
        C4[1] = '\0';

        snprintf(response, sizeof(response), "RQT OK %s %s %s %s\n", C1, C2, C3, C4);
        }
    else { snprintf(response, sizeof(response), "RQT ERR\n");}

    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr,"[ERR]: Couldn't send UDP response");
        exit(EXIT_FAILURE);
    }
    
    endGame(p);
}

int validPLID(char PLID[]) {
    return strlen(PLID) == 6;
}

void endGame(player_t *player) {
    if (player == NULL) {
        return;
    }

    // Clear the entire player structure
    memset(player, 0, sizeof(player_t));

    // Set specific default values
    strncpy(player->PLID, DEFAULT_PLAYER, sizeof(player->PLID) - 1);  
    player->PLID[sizeof(player->PLID) - 1] = '\0';                    // null termination
    player->gameStatus = 0;
    player->startTime = 0;
    player->maxTime = 0;
    player->fd = -1;
    player->attempts = 0;
}

int validTime(char time[]) {
    return (strlen(time) == 3 && atoi(time) > 1 && atoi(time) < 600);
}

void debugCommand(char input[], int fd, struct player *p, struct sockaddr *client_addr, socklen_t client_len, int verbose) {

    char CMD[4];
    char PLID[7];
    char time[4];
    char C1[2], C2[2], C3[2], C4[2];
    char response[100];

    sscanf(input, "DBG %s %s %s %s %s %s\n", PLID, time, C1, C2, C3, C4);

    // check if player has active game
    if (p->attempts > 0) { snprintf(response, sizeof(response), "RDB NOK\n");}

    else if (validPLID(PLID) && validTime(time)) { 

        // reset player stats
        endGame(p);
        strcpy(p->PLID, PLID);
        p->maxTime = atoi(time);
        p->attempts = 0;


        p->code[0] = C1[0];
        p->code[1] = C2[0];
        p->code[2] = C3[0];
        p->code[3] = C4[0];
        p->code[4] = '\0';

        p->gameStatus = 1;
        snprintf(response, sizeof(response), "RDB OK\n");
        createGameFile(p, 'D', atoi(time));
    } 
    else {
        snprintf(response, sizeof(response), "RDB ERR\n");
    }

    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr,"[ERR]: Couldn't send UDP response");
        exit(EXIT_FAILURE);
    }

}
