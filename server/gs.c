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

    player_t *player;
    strcpy(player->PLID, DEFAULT_PLAYER);

    parseArguments(argc, argv, &verbose, &GSPORT);

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
            // Timeout occurred
            printf("Timeout occurred, no activity in 10 seconds.\n");
            continue; // Restart loop
        }

        // Check UDP socket
        if (FD_ISSET(udp_fd, &testfds)) {

            int recv_bytes = recvfrom(udp_fd, buffer, sizeof(buffer), 0,
                                      (struct sockaddr *)&client_addr, &client_len);
            if (recv_bytes < 0) {
                fprintf(stderr, "[ERR]: Error receiving UDP message\n");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "[ERR]: Error forking for UDP connection\n");
            } else if (pid == 0) {
                // Child process: Handle UDP request
                handleUDPrequest(buffer, udp_fd, colorCode, player, (struct sockaddr *)&client_addr, client_len, verbose);
            }
            // Parent process continues to wait for connections
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

    sscanf(input, "%s %*s\n", CMD);
    OPCODE = parseCommand(CMD);

    switch (OPCODE)
    {
    // start
    case 1:
        startCommand(input, fd, colorCode, p, client_addr, client_len, verbose);
        break;
    // try
    case 2:
        break;
    // show trials
    case 3:
        break;
    // scoreboard
    case 4:
        break;
    // quit or exit
    case 5:
        break;
    // debug
    case 6:
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
    char time[4];
    char response[100];

    sscanf(input, "SNG %s %s\n", PLID, time);

    printf("%s\n", PLID);
    printf("%s\n", time);
    if (strlen(PLID) != 6 || strlen(time) != 3 || (atoi(time) < 1 || atoi(time) > 600)) {
        snprintf(response, sizeof(response), "RSG ERR\n");
    } if (strcmp(PLID, p->PLID) == 0) {
        snprintf(response, sizeof(response), "RSG NOK\n");
    } else {
        chooseCode(colorCode, p);
        strcpy(p->PLID, PLID);
        snprintf(response, sizeof(response), "RSG OK\n");
        printf("%s\n", p->code);
        createGameFile(p, 'P', atoi(time));
    }

    

    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
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

    printf("%s\n", p->code);
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
