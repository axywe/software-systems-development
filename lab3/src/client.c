/**
 * @file client.c
 * @brief Client application for connecting to the guessing game server over TCP/IP.
 *
 * This client application connects to a specified server running the guessing game,
 * sends user guesses, and receives responses from the server. The game continues until
 * the user either guesses the correct number or chooses to exit.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080 /**< The port number to connect to the guessing game server on. */
#define BUFFER_SIZE 1024 /**< Buffer size for receiving messages from the server. */


/**
 * @brief Main function that establishes connection to the server and processes user input.
 * 
 * Connects to the game server, handles the sending of user guesses, and processes
 * responses from the server. Continues until the user wins, loses, or exits.
 *
 * @return int Returns 0 on successful execution, or -1 on error.
 */
int main() {
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};
  char input[BUFFER_SIZE];

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  valread = read(sock, buffer, BUFFER_SIZE);
  printf("%s\n", buffer); 

  while (true) {
    printf("Enter your guess or command: ");
    fgets(input, BUFFER_SIZE, stdin);
    input[strcspn(input, "\n")] = 0; 

    if (strcmp(input, "exit") == 0) {
      break;
    }

    send(sock, input, strlen(input), 0);

    valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0'; 
    printf("Server: %s\n", buffer);

    if (strncmp(buffer, "Victory\n", sizeof("Victory\n")) == 0 ||
        strncmp(buffer, "Defeat\n", sizeof("Defeat\n")) == 0) {
      break;
    }
  }

  close(sock);
  return 0;
}
