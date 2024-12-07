#include "gs.h"

int main(int argc, char *argv[]) {

    int verbose = 0;
    char *GSPORT = NULL;
    int udp_fd, tcp_fd, client_fd;
    struct addrinfo *res_udp, *res_tcp;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];

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
                // HANDLE UDP REQUEST
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

    // Cleanup
    close(udp_fd);
    close(tcp_fd);
    freeaddrinfo(res_udp);
    freeaddrinfo(res_tcp);

    return 0;
    }
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