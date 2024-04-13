/**
 * @file server.c
 * @brief Server for a "Guess the Number" game, handling multiple clients with a
 * TCP/IP protocol.
 *
 * This server program implements the game logic for a simple "Guess the Number"
 * game, which can be played by multiple clients simultaneously. It uses TCP/IP
 * sockets to communicate with clients, and handles each client's game state
 * independently.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 256
#define INITIAL_CLIENTS 1

/**
 * @struct ClientData
 * @brief Holds individual client game data.
 *
 * @var ClientData::socket
 * The socket descriptor for the client connection.
 * @var ClientData::name
 * The client's username.
 * @var ClientData::min
 * The minimum number in the range for guessing.
 * @var ClientData::max
 * The maximum number in the range for guessing.
 * @var ClientData::secretNumber
 * The secret number the client needs to guess.
 * @var ClientData::attempts
 * The number of attempts the client has to guess the number.
 */
typedef struct {
  int socket;
  char name[BUFFER_SIZE];
  int min;
  int max;
  int secretNumber;
  int attempts;
} ClientData;

/**
 * @struct GameData
 * @brief Holds configuration data for the game.
 *
 * @var GameData::mininit
 * Initial minimum range for the secret number.
 * @var GameData::maxinit
 * Initial maximum range for the secret number.
 * @var GameData::minfin
 * Final minimum range after dynamic adjustment.
 * @var GameData::maxfin
 * Final maximum range after dynamic adjustment.
 * @var GameData::minattempts
 * Minimum number of attempts allowed per game.
 * @var GameData::maxattempts
 * Maximum number of attempts allowed per game.
 */
typedef struct {
  int mininit;
  int maxinit;
  int minfin;
  int maxfin;
  int minattempts;
  int maxattempts;
} GameData;

/**
 * @brief Accepts a new client, sets up its game data, and registers it.
 * @param server_fd Socket descriptor for the server.
 * @param address Struct containing address information of the server.
 * @param client_data Pointer to the array of client data structures.
 * @param client_capacity Pointer to the current capacity of the client data
 * array.
 * @param gameData Pointer to the game configuration data.
 * @return The socket descriptor of the newly accepted client or -1 on failure.
 */
int acceptNewClient(int server_fd, struct sockaddr_in address,
                    ClientData **client_data, int *client_capacity,
                    GameData *gameData);

/**
 * @brief Initializes the client data for new or expanding client arrays.
 * @param client_data Pointer to the client data array.
 * @param start Index from which to start initializing.
 * @param client_capacity The total capacity of the client data array.
 */
void initializeClientData(ClientData *client_data, int start,
                          int client_capacity);

/**
 * @brief Sets up the server socket, binds, and listens.
 * @param port The port on which the server should listen.
 * @return The socket descriptor for the server.
 */
int setupServerSocket(int port);

/**
 * @brief Handles the activity for a specific client.
 * @param sd The socket descriptor of the client.
 * @param client_data Pointer to the client's data structure.
 * @param address Struct containing the client's address information.
 */
void handleClientActivity(int sd, ClientData *client_data,
                          struct sockaddr_in address);

/**
 * @brief Reads game configuration data from a file.
 * @param filename Path to the configuration file.
 * @param seed Pointer to the seed value for the random number generator.
 * @param port Pointer to the port number on which the server will listen.
 * @param gameData Pointer to the structure where game data will be stored.
 * @return 0 on success, -1 on failure.
 */
int readData(const char *filename, int *seed, int *port, GameData *gameData);

/**
 * @brief Main function to execute the server logic.
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 * @return 0 on normal exit, or -1 on error.
 */
int main(int argc, char *argv[]) {
  GameData gameData;
  gameData.mininit = 1;
  gameData.maxinit = 1;
  gameData.minfin = 100;
  gameData.maxfin = 100;
  gameData.minattempts = 8;
  gameData.maxattempts = 8;
  int port = PORT;
  int seed = 1;
  if (argc != 2) {
    printf("You can use with config file: %s <path/to/conffile.txt>\n",
           argv[0]);
  } else {
    int result = readData(argv[1], &seed, &port, &gameData);
    if (result == -1) {
      return -1;
    }
  }

  int server_fd, new_socket, max_sd, sd, activity, valread,
      client_capacity = INITIAL_CLIENTS;
  ClientData *client_data =
      (ClientData *)calloc(client_capacity, sizeof(ClientData));
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];
  fd_set readfds;

  srand(seed);

  initializeClientData(client_data, 0, client_capacity);

  server_fd = setupServerSocket(port);
  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  printf("Listener on port %d \n", port);

  while (1) {
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    max_sd = server_fd;

    for (int i = 0; i < client_capacity; i++) {
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
      acceptNewClient(server_fd, address, &client_data, &client_capacity,
                      &gameData);
    }

    for (int i = 0; i < client_capacity; i++) {
      if (client_data[i].socket > 0 &&
          FD_ISSET(client_data[i].socket, &readfds)) {
        handleClientActivity(client_data[i].socket, &client_data[i], address);
      }
    }
  }

  free(client_data);
  return 0;
}

int acceptNewClient(int server_fd, struct sockaddr_in address,
                    ClientData **client_data, int *client_capacity,
                    GameData *gameData) {
  int new_socket, addrlen = sizeof(address);
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  printf("New connection, socket fd is %d, ip is : %s, port : %d \n",
         new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

  fcntl(new_socket, F_SETFL, O_NONBLOCK);

  fd_set readfds;
  struct timeval tv;
  int retval, valread;
  char name[BUFFER_SIZE] = {0};

  FD_ZERO(&readfds);
  FD_SET(new_socket, &readfds);

  tv.tv_sec = 5;
  tv.tv_usec = 0;

  retval = select(new_socket + 1, &readfds, NULL, NULL, &tv);

  if (retval == -1) {
    perror("select()");
    close(new_socket);
    return -1;
  } else if (retval) {
    valread = recv(new_socket, name, BUFFER_SIZE - 1, 0);
    if (valread > 0) {
      name[valread] = '\0';
      printf("Valread: %d, Name: %s\n", valread, name);
    } else {
      if (valread == 0) {
        printf("Connection closed\n");
      } else {
        perror("read");
      }
      close(new_socket);
      return -1;
    }
  } else {
    printf("No data within the timeout period.\n");
    close(new_socket);
    return -1;
  }

  int added = -1;
  for (int i = 0; i < *client_capacity; i++) {
    printf("Comparing %s with %s\n", (*client_data)[i].name, name);
    if (strcmp((*client_data)[i].name, name) == 0) {
      char *message = "u";
      send(new_socket, message, strlen(message), 0);
      printf("Username already taken\n");
      close(new_socket);
      return -1;
    }
  }
  for (int i = 0; i < *client_capacity; i++) {
    if ((*client_data)[i].socket == 0) {
      (*client_data)[i].socket = new_socket;
      (*client_data)[i].min =
          gameData->mininit +
          rand() % (gameData->maxinit - gameData->mininit + 1);
      (*client_data)[i].max =
          gameData->minfin + rand() % (gameData->maxfin - gameData->minfin + 1);
      (*client_data)[i].attempts =
          gameData->minattempts +
          rand() % (gameData->maxattempts - gameData->minattempts + 1);
      (*client_data)[i].secretNumber =
          (*client_data)[i].min +
          rand() % ((*client_data)[i].max - (*client_data)[i].min + 1);
      strncpy((*client_data)[i].name, name, valread);
      printf(
          "Adding to list of sockets as %d with secret number %d, range: %d "
          "- %d\n",
          i, (*client_data)[i].secretNumber, (*client_data)[i].min,
          (*client_data)[i].max);
      added = i;
      break;
    }
  }

  if (added == -1) {
    *client_capacity *= 2;
    *client_data = (ClientData *)realloc(*client_data,
                                         *client_capacity * sizeof(ClientData));
    initializeClientData(*client_data, *client_capacity / 2, *client_capacity);
    int i = *client_capacity / 2;
    (*client_data)[i].socket = new_socket;
    (*client_data)[i].min =
        gameData->mininit +
        rand() % (gameData->maxinit - gameData->mininit + 1);
    (*client_data)[i].max =
        gameData->minfin + rand() % (gameData->maxfin - gameData->minfin + 1);
    (*client_data)[i].attempts =
        gameData->minattempts +
        rand() % (gameData->maxattempts - gameData->minattempts + 1);
    (*client_data)[i].secretNumber =
        (*client_data)[i].min +
        rand() % ((*client_data)[i].max - (*client_data)[i].min + 1);
    strncpy((*client_data)[i].name, name, valread);
    printf("Resized client data to %d\n", *client_capacity);
    printf(
        "Adding to list of sockets as %d with secret number %d, range: %d - "
        "%d\n",
        i, (*client_data)[i].secretNumber, (*client_data)[i].min,
        (*client_data)[i].max);
    added = i;
  }

  char *message = (char *)malloc(BUFFER_SIZE);
  sprintf(message, "h %d %d %d", (*client_data)[added].min,
          (*client_data)[added].max, (*client_data)[added].attempts);
  send(new_socket, message, strlen(message), 0);
  printf("User accepted, send message: %s, %d, %d\n", message,
         *client_capacity - 1, added);
  free(message);

  return new_socket;
}

void initializeClientData(ClientData *client_data, int start,
                          int client_capacity) {
  for (int i = start; i < client_capacity; i++) {
    client_data[i].socket = 0;
    client_data[i].name[0] = '\0';
    client_data[i].min = 0;
    client_data[i].max = 0;
    client_data[i].secretNumber = rand() % 100 + 1;
    client_data[i].attempts = 0;
  }
}

int setupServerSocket(int port) {
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

void handleClientActivity(int sd, ClientData *client_data,
                          struct sockaddr_in address) {
  int valread, addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];
  if ((valread = recv(sd, buffer, BUFFER_SIZE, 0)) == 0) {
    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr),
           ntohs(address.sin_port));
    close(sd);
    client_data->socket = 0;
    client_data->name[0] = '\0';
  } else {
    buffer[valread] = '\0';
    int guessedNumber;
    char command[20];
    if (sscanf(buffer, "%s %d", command, &guessedNumber) == 2) {
      printf("Client %d: %s, Secret: %d, Attempts: %d Min: %d Max: %d\n",
             client_data->socket, buffer, client_data->secretNumber,
             client_data->attempts, client_data->min, client_data->max);
      if (strcmp(command, "g") == 0 && client_data->attempts > 0) {
        send(sd, client_data->secretNumber > guessedNumber ? "c" : "i", 1, 0);
        client_data->attempts--;
      } else if (strcmp(command, "l") == 0 && client_data->attempts > 0) {
        send(sd, client_data->secretNumber < guessedNumber ? "c" : "i", 1, 0);
        client_data->attempts--;
      } else if (strcmp(command, "e") == 0) {
        if (client_data->secretNumber == guessedNumber) {
          send(sd, "v", strlen("v"), 0);
          getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
          printf("Victory! Host disconnected, ip %s, port %d \n",
                 inet_ntoa(address.sin_addr), ntohs(address.sin_port));
          close(sd);
          client_data->socket = 0;
          client_data->name[0] = '\0';
        } else {
          send(sd, "d", strlen("d"), 0);
          getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
          printf("Defeat! Host disconnected, ip %s, port %d \n",
                 inet_ntoa(address.sin_addr), ntohs(address.sin_port));
          close(sd);
          client_data->socket = 0;
          client_data->name[0] = '\0';
        }
      } else if (strcmp(command, "g") == 0 || strcmp(command, "l") == 0) {
        send(sd, "o", 1, 0);
      } else {
        send(sd, "q", 1, 0);
      }
    } else {
      send(sd, "f", 1, 0);
    }
  }
}

int readData(const char *filename, int *seed, int *port, GameData *gameData) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    printf("File not found\n");
    return -1;
  }
  if (fscanf(file, "%d", seed) != 1 ||
      fscanf(file, "%d", port) != 1 ||
      fscanf(file, "%d", &gameData->minattempts) != 1 ||
      fscanf(file, "%d", &gameData->maxattempts) != 1 ||
      fscanf(file, "%d", &gameData->mininit) != 1 ||
      fscanf(file, "%d", &gameData->maxinit) != 1 ||
      fscanf(file, "%d", &gameData->minfin) != 1 ||
      fscanf(file, "%d", &gameData->maxfin) != 1) {
    printf("Error reading data. Ensure all 8 integers are present.\n");
    fclose(file);
    return -1;
  }

  char buffer;
  if (fscanf(file, "%c", &buffer) != EOF && buffer != '\n' && buffer != ' ') {
    printf("Extraneous data detected in file.\n");
    fclose(file);
    return -1;
  }

  fclose(file);
  if (*port < 1024 || *port > 65535) {
    printf("Port must be in range 1024-65535\n");
    return -1;
  }

  if (gameData->minattempts < 1 || gameData->maxattempts < 1 ||
      gameData->mininit < 1 || gameData->maxinit < 1 || gameData->minfin < 1 ||
      gameData->maxfin < 1) {
    printf("All values must be greater than 0\n");
    return -1;
  }

  if (gameData->minattempts > gameData->maxattempts ||
      gameData->mininit > gameData->maxinit ||
      gameData->minfin > gameData->maxfin) {
    printf("Min values must be less than max values\n");
    return -1;
  }
  return 0;
}