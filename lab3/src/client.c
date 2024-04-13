/**
 * @file client.c
 * @brief Client for a "Guess the Number" game, interacting with a game server
 * using TCP/IP protocol.
 *
 * This client program interacts with a game server to play a simple "Guess the
 * Number" game. It uses TCP/IP sockets to send guesses and receive responses,
 * and provides a text-based interface for the user to play the game.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define PORT 8080

/**
 * @brief Parses and validates command line arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param host Pointer to host address string.
 * @param port Pointer to port number.
 * @param name Pointer to username string.
 * @return True if arguments are valid, false otherwise.
 */
bool parse_and_validate_args(int argc, char *argv[], char **host, int *port,
                             char **name) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      *host = argv[i + 1];
    } else if (strcmp(argv[i], "-p") == 0) {
      *port = atoi(argv[i + 1]);
    } else if (strcmp(argv[i], "-n") == 0) {
      *name = argv[i + 1];
    }
  }

  if (*host == NULL) *host = "127.0.0.1";
  if (*port == 0) *port = PORT;
  if (*name == NULL) {
    printf("Usage: %s -h <host> -p <port> -n <name>\n", argv[0]);
    return false;
  }
  if (*port < 1024 || *port > 65535) {
    printf("Port must be in range 1024-65535\n");
    return false;
  }
  if (strlen(*name) > 255) {
    printf("Name must be less than 255 characters\n");
    return false;
  }

  return true;
}

/**
 * @brief Parses the client's textual input into game-specific commands.
 * @param input User's textual input string.
 * @return A string containing a game command or error indicator.
 */
char *parse_client_input(const char *input) {
  static char command[100];
  int number;
  char checkChar;

  if (sscanf(input, "greater than %d%c", &number, &checkChar) == 1) {
    sprintf(command, "g %d", number);
  } else if (sscanf(input, "less than %d%c", &number, &checkChar) == 1) {
    sprintf(command, "l %d", number);
  } else if (sscanf(input, "equal %d%c", &number, &checkChar) == 1) {
    sprintf(command, "e %d", number);
  } else if (strcmp(input, "exit") == 0) {
    strcpy(command, "exit");
  } else {
    strcpy(command, "i");
  }
  if (number < 0 || strlen(input) > 22) {
    strcpy(command, "i");
  }
  return command;
}

/**
 * @brief Parses the server's single character responses into human-readable
 * strings.
 * @param response Character response from server.
 * @return Human-readable string describing the server's response.
 */
char *parse_server_response(char response) {
  switch (response) {
    case 'c':
      return "Correct guess.";
    case 'i':
      return "Incorrect guess.";
    case 'v':
      return "Victory!";
    case 'd':
      return "Defeat.";
    case 'f':
      return "Incorrect question format.";
    case 'q':
      return "Invalid question.";
    case 'o':
      return "Out of attempts.";
    default:
      return "Unknown response.";
  }
}

/**
 * @brief Displays the current state of the game including user inputs and
 * server responses.
 * @param username The user's name.
 * @param attempts The number of attempts remaining.
 * @param min The minimum number in the guessing range.
 * @param max The maximum number in the guessing range.
 * @param last_input The last input given by the user.
 * @param last_response The last response received from the server.
 */
void display_game_state(const char *username, int attempts, int min, int max,
                        const char *last_input, const char *last_response) {
  printf("\033[2J\033[H");
  printf("\033[1;31mWelcome to the number guessing game!\033[0m\n");
  printf("\033[1;30mCommands:\033[0m\n");
  printf("\033[1;30mgreater than <positive integer>\033[0m\n");
  printf("\033[1;30mlower than <positive integer>\033[0m\n");
  printf("\033[1;30mequal <positive integer>\033[0m\n");
  printf("\033[1;34mUsername: %.10s\033[0m\n", username);
  printf("\033[1;32mAttempts remaining: %3d\033[0m\n", attempts);
  printf("\033[1;35mRange: %d - %d\033[0m\n", min, max);
  printf("\033[1;33mLast Input: %s\033[0m\n", last_input);
  printf("\033[1;33mLast Server Response: %s\033[0m\n", last_response);
  printf("\033[1;33m----------------------------------------\033[0m\n");
  printf("\033[1;36mEnter your guess or command: \033[0m");
}

/**
 * @brief Main loop for the game, handling input, output, and communication with
 * the server.
 * @param sock Socket descriptor connected to the server.
 * @param attempts Number of attempts given to guess the number.
 * @param min Minimum value in the number range.
 * @param max Maximum value in the number range.
 * @param name User's name.
 */
void game_loop(int sock, int attempts, int min, int max, char *name[]) {
  char buffer[BUFFER_SIZE];
  char input[BUFFER_SIZE];
  char last_input[BUFFER_SIZE] = "";
  char last_response[BUFFER_SIZE] = "";

  while (true) {
    display_game_state(*name, attempts, min, max, last_input, last_response);
    fgets(input, BUFFER_SIZE, stdin);
    input[strcspn(input, "\n")] = 0;
    char *parsed_input = parse_client_input(input);
    if (strcmp(parsed_input, "exit") == 0) {
      break;
    }

    strcpy(last_input, input);
    if (strcmp(parsed_input, "i") == 0) {
      sprintf(last_response, "Invalid input");
      continue;
    }

    send(sock, parsed_input, strlen(parsed_input), 0);

    int valread = recv(sock, buffer, 1, 0);
    if (valread == 0 || valread == -1) {
      printf("Server disconnected\n");
      break;
    }
    if (buffer[0] == 'c' || buffer[0] == 'i') {
      attempts--;
    }
    strcpy(last_response, parse_server_response(buffer[0]));

    if (strncmp(buffer, "v", 1) == 0 || strncmp(buffer, "d", 1) == 0) {
      printf("%s\n", parse_server_response(buffer[0]));
      break;
    }
  }
}

/**
 * @brief Main function for the client application.
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 * @return 0 on normal exit, or -1 on error.
 */
int main(int argc, char *argv[]) {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};

  char *host = NULL;
  int port = 0;
  char *name = NULL;

  if (!parse_and_validate_args(argc, argv, &host, &port, &name)) {
    return -1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  send(sock, name, strlen(name) + 1, 0);
  int valread = recv(sock, buffer, BUFFER_SIZE, 0);
  int min, max, attempts;
  if (valread == 0) {
    printf("Server disconnected\n");
    return -1;
  } else if (buffer[0] == 'h') {
    if (sscanf(buffer, "h %d %d %d", &min, &max, &attempts) != 3) {
      printf("Server error\n");
      return -1;
    }
  } else if (buffer[0] == 'u') {
    printf("Name already exists\n");
    return -1;
  }

  game_loop(sock, attempts, min, max, &name);

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