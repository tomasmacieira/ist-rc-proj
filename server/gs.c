#include "gs.h"


// Array to store games
player_t Games[999999] = {0}; // Statically allocated array for players

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
    player_t * player;
    player_t p;
    strcpy(p.PLID, DEFAULT_PLAYER);
    p.attempts = 0;
    p.gameStatus = 0;
    Games[123456] = p;
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
                handleTCPrequest(client_fd, colorCode, player, verbose);
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
    
    //printf("input: %s", input);
    sscanf(input, "%s %*s\n", CMD);
    OPCODE = parseCommand(CMD);
    switch (OPCODE) {
    // start
    case 1:
        startCommand(input, fd, colorCode, client_addr, client_len, verbose);
        break;
    // try
    case 2:
        tryCommand(input, fd, colorCode, client_addr, client_len, verbose);
        break;
    // quit or exit
    case 5:
        quitCommand(input, fd, client_addr, client_len, verbose);
        break;
    // debug
    case 6:
        debugCommand(input, fd, client_addr, client_len, verbose);
        break;
    default:
        break;
    }

}

void handleTCPrequest(int client_fd, int colorCode[], struct player *p, int verbose) {
    char input[1024], command[4];
    int OPCODE;
    ssize_t bytesRead;

    memset(input, 0, sizeof(input));
    memset(command, 0, sizeof(command));

    // Read the command from the client
    bytesRead = read(client_fd, input, sizeof(input) - 1);
    if (bytesRead <= 0) {
        perror("[ERR]: Failed to read from TCP client");
        close(client_fd);
        return;
    }

    // Parse the command
    sscanf(input, "%s %*s\n", command);
    OPCODE = parseCommand(command);

    switch (OPCODE) {
    // Show trials
    case 3:
        showtrialsCommand(client_fd, p, verbose);
        break;
    // Scoreboard
    case 4:
        break;
    default:
        // Handle unknown commands
        fprintf(stderr, "[ERR]: Unknown TCP command received: %s\n", command);
        const char *errorResponse = "ERR Unknown command\n";
        write(client_fd, errorResponse, strlen(errorResponse));
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

void startCommand(char input[], int fd, int colorCode[], struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    char PLID[7];
    char gameTime[4];
    char response[100];
    char code[5];

    sscanf(input, "SNG %s %s\n", PLID, gameTime);

    // Validate PLID and gameTime
    if (strlen(PLID) != 6 || strlen(gameTime) != 3 || (atoi(gameTime) < 1 || atoi(gameTime) > 600)) {
        snprintf(response, sizeof(response), "RSG ERR\n");
    } else if (Games[atoi(PLID)].gameStatus == 1) {
        // Check if the game is already active
        snprintf(response, sizeof(response), "RSG NOK\n");
    } else {
        // Initialize the game directly in Games array
        player_t *player = &Games[atoi(PLID)]; // Access the `player_t` object in the array
        memset(player, 0, sizeof(player_t));  // Reset the player's data
        strcpy(player->PLID, PLID);
        player->attempts = 0;
        player->maxTime = atoi(gameTime);
        player->gameStatus = 1;
        strcpy(player->gameMode, "Play");

        // Generate the code for the game
        chooseCode(colorCode, player);

        // Create a game file
        createGameFile(player, 'P', atoi(gameTime));

        // Send the response
        snprintf(response, sizeof(response), "RSG OK\n");
    }

    // Print verbose information
    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    // Send the response to the client
    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP response");
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

void showtrialsCommand(int client_fd, struct player *p, int verbose) {
    char buffer[1024], response[256], Fname[64], fullPath[128];
    ssize_t bytesWritten, bytesRead;
    int trialFile;
    off_t fileSize;
    const char *status;
    if(p->gameStatus){
        status = "ACT";
    }
    else{
        status = "FIN";
    }

    // Prepare file path: games/PLID/GAME_PLID.txt
    snprintf(fullPath, sizeof(fullPath), "server/games/%s/GAME_%s.txt", p->PLID, p->PLID);
    snprintf(Fname, sizeof(Fname), "GAME_%s.txt", p->PLID);
    // Attempt to open the file
    trialFile = open(fullPath, O_RDONLY);
    if (trialFile < 0) {
        // File not found or no trials
        snprintf(response, sizeof(response), "RST NOK\n");
        bytesWritten = write(client_fd, response, strlen(response));
        if (bytesWritten < 0) {
            fprintf(stderr, "[ERR]: Failed to send response to client.\n");
        }
        close(client_fd);
        return;
    }

    // Get file size
    fileSize = lseek(trialFile, 0, SEEK_END);
    if (fileSize == -1) {
        perror("[ERR]: Failed to determine file size");
        close(trialFile);
        close(client_fd);
        return;
    }
    lseek(trialFile, 0, SEEK_SET);

// Send response header with status, file name, and file size
    snprintf(response, sizeof(response), "RST %s %s %ld\n", status, Fname, fileSize);
    bytesWritten = write(client_fd, response, strlen(response));
    if (bytesWritten < 0) {
        fprintf(stderr, "[ERR]: Failed to send response header to client.\n");
        close(trialFile);
        close(client_fd);
        return;
    }

    // Send the file content in chunks
    while ((bytesRead = read(trialFile, buffer, sizeof(buffer))) > 0) {
        bytesWritten = write(client_fd, buffer, bytesRead);
        if (bytesWritten < 0) {
            fprintf(stderr, "[ERR]: Failed to send file content to client.\n");
            break;
        }
    }

    if (verbose) {
        printf("[INFO]: Sent trials file '%s' for player '%s'.\n", Fname, p->PLID);
    }

    // Close file and client connection
    close(trialFile);
    close(client_fd);
}

void tryCommand(char input[], int fd, int colorCode[], struct sockaddr *client_addr, socklen_t client_len, int verbose) {
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
    sscanf(input, "TRY %s %s %s %s %s %d\n", PLID, C1, C2, C3, C4, &attempt);
    
    int id = atoi(PLID); // Convert PLID to an integer index
    player_t *p = &Games[id]; // Directly access the player_t struct in the array
    p->attempts++;
    memset(try, 0, sizeof(try));
     // register try
    strcpy(try, C1);
    strcat(try, C2);
    strcat(try, C3);
    strcat(try, C4);
    printf("Here: %s", p->PLID);

    if(strcmp(p->PLID,DEFAULT_PLAYER)==0 || p->gameStatus != 1){
        snprintf(response, sizeof(response), "RTR NOK\n");
        p->attempts--;
    }
    else if (checkColors(C1[0], C2[0], C3[0], C4[0]) || strlen(p->PLID) != 6) {
        snprintf(response, sizeof(response), "RTR ERR\n");
        p->attempts--;
    }
    else if(difftime(currentTime, p->startTime) > p->maxTime) {
        snprintf(response, sizeof(response), "RTR ETM %c %c %c %c\n", p->code[0], p->code[1], p->code[2], p->code[3]);
        p->attempts--;
        endGame(p);
    }

    else if(p->attempts != attempt && !(p->attempts - 1 == attempt && checkPreviousTry(p, try))){
        snprintf(response, sizeof(response), "RTR INV\n");
        p->attempts--;
    }

    else if (p->attempts > 8) {
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

        int sameColor[4] = {0, 0, 0, 0};                        // Register for correct colors in diferent positions
        int correctColorsPositions[4] = {0, 0, 0, 0};           // register correct colors in correct positions

        // find nB (correct colors in correct positions)
        for (int i = 0; i < 4; i++) {
            if (try[i] == p->code[i]) {
                nB++;
                sameColor[i] = 1;                               // Register as seen
                correctColorsPositions[i] = 1;
            }
        }

        // find nW (correct color in wrong positions)
        for (int i = 0; i < 4; i++) {
            if (correctColorsPositions[i]) continue;            // this color increased nB 
            for (int j = 0; j < 4; j++) {
                if (!sameColor[j] && try[i] == p->code[j]) {
                    nW++;
                    sameColor[j] = 1;                           // Mark this position in code as matched
                    break;
                }
            }
        }
        snprintf(response, sizeof(response), "RTR OK %d %d %d\n", p->attempts, nB, nW);
        strcpy(p->tries[p->attempts - 1], try);
        writeTry(p, nB, nW);
        if(nB == 4){
            saveGameScore(p);
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

void saveGameScore(struct player *p) {
    char filepath[100];
    char date[20]; 
    char hours[20];
    char firstLine[128];
    time_t fulltime;
    time_t currentTime = time(NULL);
    struct tm *current_time;
    char scoreStr[4];

    time(&fulltime);

    current_time = gmtime(&fulltime);

    // format date and hours
    snprintf(date, sizeof(date), "%4d%02d%02d",
             current_time->tm_year + 1900,
             current_time->tm_mon + 1,
             current_time->tm_mday);

    snprintf(hours, sizeof(hours), "%02d%02d%02d",
             current_time->tm_hour,
             current_time->tm_min,
             current_time->tm_sec);

    // calculate time taken
    int timeTaken = (int)(currentTime - p->startTime); 

    // calculate score
    int score = (((600 - timeTaken) / 600.0) * 70) + (((8 - p->attempts) / 8.0) * 30);
    if (score < 0) score = 0;
    if (score > 100) score = 100; 

    // convert score to a 3-digit string
    snprintf(scoreStr, sizeof(scoreStr), "%03d", score);

    // format: <score PLID DDMMYYYY HHMMSS.txt>
    snprintf(filepath, sizeof(filepath), "./server/scores/%s_%s_%s_%s.txt", scoreStr, p->PLID, date, hours);

    p->score_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (p->score_fd == -1) {
        fprintf(stderr, "[ERR]: open failed\n");
        exit(EXIT_FAILURE);
    }

    // Write the first line to the file
    snprintf(firstLine, sizeof(firstLine),
             "%s %s %s %d %s\n",
             scoreStr,          // Score
             p->PLID,           // PLID
             p->code,           // Secret code to win
             p->attempts,       // Number of attempts
             p->gameMode);      // Mode (play or debug)

    if (write(p->score_fd, firstLine, strlen(firstLine)) == -1) {
        fprintf(stderr, "[ERR]: write failed\n");
        close(p->score_fd);
        exit(EXIT_FAILURE);
    }

    close(p->score_fd); 
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
             "%-6s %c %s %d %s %s %lds\n",
             p->PLID,           // PLID
             mode,              // mode (Play ou Debug)
             p->code,           // secret code to win 
             timeLimit,         // time to win
             date,              // YYYY-MM-DD
             hours,             // HH:MM:SS
             p->maxTime);         // time when game started

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

    snprintf(line, sizeof(line), "Trial: %s nB: %d nW: %d  %ds\n", p->tries[p->attempts - 1], nB, nW, (int)difftime(time(NULL), p->startTime));

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

void quitCommand(char input[], int fd, struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    char PLID[7];
    char response[100];
    char C1[2]; char C2[2]; char C3[2]; char C4[2];

    sscanf(input, "QUT %s\n", PLID);

    // Get the player from the Games array using the PLID as index
    int id = atoi(PLID); // Convert PLID to an integer index
    player_t *p = &Games[id]; // Access the corresponding player object in the array

    // Check if the player has an ongoing game
    if (p->gameStatus == 0 || !validPLID(PLID)) {
        snprintf(response, sizeof(response), "RQT NOK\n");
    }
    else if (p->gameStatus == 1) { 
        // Save the player's code before ending the game
        C1[0] = p->code[0];
        C1[1] = '\0';  

        C2[0] = p->code[1];
        C2[1] = '\0';

        C3[0] = p->code[2];
        C3[1] = '\0';

        C4[0] = p->code[3];
        C4[1] = '\0';

        // End the game for this player
        endGame(p);

        // Prepare response indicating successful game termination
        snprintf(response, sizeof(response), "RQT OK %s %s %s %s\n", C1, C2, C3, C4);
    }
    else { 
        // If no valid player or ongoing game, return error response
        snprintf(response, sizeof(response), "RQT ERR\n");
    }

    // Optionally print debug information if verbose mode is enabled
    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    // Send the response back to the client
    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP response\n");
        exit(EXIT_FAILURE);
    }
    
    // Additional end game call after responding (cleanup, if necessary)
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
    memset(player->gameMode, 0, sizeof(player->gameMode));
    player->gameStatus = 0;
    player->startTime = 0;
    player->maxTime = 0;
    player->fd = -1;
    player->score_fd = -1;
    player->attempts = 0;
}

int validTime(char time[]) {
    return (strlen(time) == 3 && atoi(time) >= 1 && atoi(time) <= 600);
}

void debugCommand(char input[], int fd, struct sockaddr *client_addr, socklen_t client_len, int verbose) {
    char PLID[7];
    char time[4];
    char C1[2], C2[2], C3[2], C4[2];
    char response[100];

    // Parse the command
    sscanf(input, "DBG %s %s %s %s %s %s\n", PLID, time, C1, C2, C3, C4);

    int id = atoi(PLID); // Convert PLID to an integer index
    player_t *p = &Games[id]; // Access the corresponding player object in the array

    // Check if the player has an active game
    if (p->gameStatus == 1 && p->attempts > 0) {
        snprintf(response, sizeof(response), "RDB NOK\n");
    } else if (validPLID(PLID) && validTime(time)) { 
        // Reset player stats
        endGame(p); // Clean up any existing game state
        strcpy(p->PLID, PLID);
        strcpy(p->gameMode, "Debug");
        p->maxTime = atoi(time);
        p->attempts = 0;

        // Set the new debug code
        p->code[0] = C1[0];
        p->code[1] = C2[0];
        p->code[2] = C3[0];
        p->code[3] = C4[0];
        p->code[4] = '\0';

        // Update game status and create a debug game file
        p->gameStatus = 1;
        snprintf(response, sizeof(response), "RDB OK\n");
        createGameFile(p, 'D', atoi(time));
    } else {
        snprintf(response, sizeof(response), "RDB ERR\n");
    }

    // Optionally print debug info if verbose mode is enabled
    if (verbose) {
        printDescription(input, PLID, client_addr, client_len);
    }

    // Send the response back to the client
    if (sendto(fd, response, strlen(response), 0, client_addr, client_len) == -1) {
        fprintf(stderr, "[ERR]: Couldn't send UDP response\n");
        exit(EXIT_FAILURE);
    }
}

