#ifndef PLAYER_H
#define PLAYER_h

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
#define DEFAULT_PLAYER "000000"

/**
 * Parses the IP address and port from the command-line arguments.
 *
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments.
 * @param GSIP A pointer to store the parsed IP address.
 * @param GSPORT A pointer to store the parsed port number.
 */
void parseArguments(int argc, char *argv[], char **GSIP, char **GSPORT);

/**
 * Creates a UDP socket using the provided IP address and port.
 *
 * @param GSIP The IP address to bind the socket to.
 * @param GSPORT The port number to bind the socket to.
 * @param res A pointer to store the result of the address lookup.
 * @return UDP Socket descriptor
 */
int createUDPSocket(const char *GSIP, const char *GSPORT, struct addrinfo **res);

/**
 * Creates a TCP socket using the provided IP address and port.
 *
 * @param GSIP The IP address to bind the socket to.
 * @param GSPORT The port number to bind the socket to.
 * @param res A pointer to store the result of the address lookup.
 * @return TCP Socket descriptor
 */
int createTCPSocket(const char *GSIP, const char *GSPORT, struct addrinfo **res);

/**
 * Determines the command given.
 *
 * @param command Pointer to input string given by the player.
 * @return Comand OPCODE
 */
int getCommand(char* command);

/**
 * Sends a "start" command to start a game with the provided player ID and time.
 *
 * @param input The input string containing the command, player ID, and time.
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 * @param player The player's ID.
 * @param trialCount The trial count
 */
void startCommand(char input[], int fd, struct addrinfo *res, char player[], int *trialCount);

/**
 * Sends a "try" command to the server with the given color code
 *
 * @param input The input string containing the command and color code.
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 * @param PLID The player's ID.
 * @param trialCount The current trial.
 */
void tryCommand(char input[], int fd, struct addrinfo *res, char PLID[], int *trialCount);


/**
 * Sends a "quit" command to quit the game and/or close the connection with the server.
 *
 * @param input The input string containing the command.
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 * @param player The player's ID.
 * @param exitCommand If non-zero, exits the program and closes the connection.
 */
void quitCommand(char input[], int fd, struct addrinfo *res, char player[], int exitCommand);

/**
 * Sends a "debug" command to the server for debugging purposes.
 *
 * @param input The input string containing the command and guess.
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 * @param player The player's ID.
 */
void debugCommand(char input[], int fd, struct addrinfo *res, char player[]);

/**
 * Sends a "scoreboard" command to retrieve the scoreboard and saves the response to a file.
 *
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 */
void scoreboardCommand(int fd, struct addrinfo *res);

/**
 * Sends a "show trials" command to retrieve the player's trial data and saves the response to a file.
 *
 * @param fd The socket file descriptor.
 * @param res The address information for the server.
 * @param player The player's ID.
 */
void showtrialsCommand(int fd, struct addrinfo *res, char player[]);

#endif