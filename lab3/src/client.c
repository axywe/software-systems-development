#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};
  char input[BUFFER_SIZE];

  char *host = NULL;
  int port = 0;
  char *name = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      host = argv[i + 1];
    } else if (strcmp(argv[i], "-p") == 0) {
      port = atoi(argv[i + 1]);
    } else if (strcmp(argv[i], "-n") == 0) {
      name = argv[i + 1];
    }
  }

  if (host == NULL) {
    host = "127.0.0.1";
  }
  if (port == 0) {
    port = 8080;
  }
  if (name == NULL) {
    printf("Usage: %s -h <host> -p <port> -n <name>\n", argv[0]);
    return -1;
  }
  if (port < 1024 || port > 65535) {
    printf("Port must be in range 1024-65535\n");
    return -1;
  }
  if (strlen(name) > 255) {
    printf("Name must be less than 255 characters\n");
    return -1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  send(sock, name, strlen(name)+1, 0);
  valread = read(sock, buffer, BUFFER_SIZE);
  int min, max, attempts;
  if (valread == 0) {
    printf("Server disconnected\n");
    return -1;
  } else if (buffer[0] == 'h') {
    if(sscanf(buffer, "h %d %d %d", &min, &max, &attempts) == 3){
      printf("%s, welcome to the game! You have %d attempts to guess the number between %d and %d\n", name, attempts, min, max);
    }else{
      printf("Server error\n");
      return -1;
    }
  } else if (buffer[0] == 'u') {
    printf("Name already exists\n");
    return -1;
  }

  while (true) {
    printf("Enter your guess or command: ");
    fgets(input, BUFFER_SIZE, stdin);
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "exit") == 0) {
      break;
    }

    send(sock, input, strlen(input), 0);
    printf("Sent: %s\n", input);

    valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0';
    printf("Server: %s\n", buffer);

    if (strncmp(buffer, "v", 1) == 0 || strncmp(buffer, "d", 1) == 0) {
      break;
    }
  }

  close(sock);
  return 0;
}

// 1. Клиент отправляет имя
// 2. Приветственное сообщение с клиентской стороны
// 3. Сервер принимает конфигурационный файл, в котором указан seed, два числа,
// которые задают диапазон нижней границы, два числа для верхней границы, порт
// 4. Только положительные числа
// Каждые 10 запросов показывать пользователю условие, количество попыток
// 5. Отправка диапазона клиенту
// Запуск клиента - ./client -h <host> -p <port> -n <name>
// Запуск сервера - ./server conffile.txt

// ./a.out

// 10
// 8080

// 3
// 10

// 34
// 55

// 1000
// 5000