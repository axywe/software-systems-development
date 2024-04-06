/**
 * @file server.c
 * @brief Server application for a multi-client guessing game over TCP/IP.
 *
 * This server application listens on a specified port for incoming TCP connections.
 * Each connected client is asked to guess a secret number. The server responds to
 * guesses with hints or a notification of correct guess. Supports multiple clients
 * concurrently using non-blocking sockets and select() for multiplexing.
 */
#include <arpa/inet.h> 
#include <errno.h>     
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8080 /**< Port number for the server to listen on. */
#define MAX_CLIENTS 65536 /**< Maximum number of clients that can connect simultaneously. */
#define BUFFER_SIZE 1024 /**< Size of the buffer for reading messages from clients. */

/**
 * @struct ClientData
 * @brief Holds information about a connected client.
 *
 * @var ClientData::socket
 * Socket descriptor for the client.
 * @var ClientData::secretNumber
 * The secret number assigned to this client to guess.
 */

typedef struct {
    int socket;
    int secretNumber;
} ClientData;

/**
 * @brief Entry point of the server application.
 * 
 * Initializes the server, sets up listening socket and enters the main loop
 * to accept new connections and handle incoming messages from clients.
 *
 * @return int Returns 0 on successful execution.
 */

int main() {
    int server_fd, new_socket, max_sd, sd, activity, valread;
    ClientData client_data[MAX_CLIENTS];
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    srand(time(NULL)); 

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_data[i].socket = 0; 
        client_data[i].secretNumber = 0; 
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listener on port %d \n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_data[i].socket;
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is : %s, port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            fcntl(new_socket, F_SETFL, O_NONBLOCK);
            char *message = "Welcome to the Guessing Game! \n";
            send(new_socket, message, strlen(message), 0);
            printf("Welcome message sent successfully\n");

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_data[i].socket == 0) {
                    client_data[i].socket = new_socket;
                    client_data[i].secretNumber = rand() % 100 + 1; 
                    printf("Adding to list of sockets as %d with secret number %d\n", i, client_data[i].secretNumber);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_data[i].socket;

            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_data[i].socket = 0; 
                } else {
                    buffer[valread] = '\0';
                    int guessedNumber;
                    char command[20];
                    if (sscanf(buffer, "number X %s %d?", command, &guessedNumber) == 2) {
                        if (strcmp(command, "greater") == 0) {
                            send(sd, client_data[i].secretNumber > guessedNumber ? "Correct\n" : "Incorrect\n", sizeof("Incorrect\n"), 0);
                        } else if (strcmp(command, "less") == 0) {
                            send(sd, client_data[i].secretNumber < guessedNumber ? "Correct\n" : "Incorrect\n", sizeof("Incorrect\n"), 0);
                        } else if (strcmp(command, "equal") == 0) {
                            if (client_data[i].secretNumber == guessedNumber) {
                                send(sd, "Victory\n", sizeof("Victory\n"), 0);
                                printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                                close(sd);
                                client_data[i].socket = 0;
                            } else {
                                send(sd, "Defeat\n", sizeof("Defeat\n"), 0);
                                printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                                close(sd);
                                client_data[i].socket = 0;
                            }
                        } else {
                            send(sd, "incorrect question\n", sizeof("incorrect question\n"), 0);
                        }
                    } else {
                        send(sd, "incorrect question format\n", sizeof("incorrect question format\n"), 0);
                    }
                }
            }
        }
    }

    return 0;
}
