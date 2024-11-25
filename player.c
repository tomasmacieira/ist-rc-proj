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

void startCommand(char input[]) {
    char *rest = strchr(input, ' ');
    rest++;
    printf("%s\n", rest);
}

int main(int argc, char *argv[]) {
    
    char *GSIP = NULL;
    char *GSPORT = NULL;
    
    int OPCODE;
    char line[128];
    char input[128];

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
                startCommand(input);
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
                exit(1);
            case 6:
                printf("exit\n");
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