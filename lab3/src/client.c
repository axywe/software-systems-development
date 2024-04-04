// client.c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

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
  printf("%s\n", buffer); // Welcome message

  while (true) {
    printf("Enter your guess or command: ");
    fgets(input, BUFFER_SIZE, stdin);
    input[strcspn(input, "\n")] = 0; // Remove newline character

    if (strcmp(input, "exit") == 0) {
      break;
    }

    send(sock, input, strlen(input), 0);

    valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0'; // Null terminate the buffer
    printf("Server: %s\n", buffer);

    if (strncmp(buffer, "Victory\n", 8) == 0 ||
        strncmp(buffer, "Defeat\n", 7) == 0) {
      break;
    }
  }

  close(sock);
  return 0;
}
