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

typedef struct {
  int socket;
  char name[BUFFER_SIZE];
  int min;
  int max;
  int secretNumber;
  int attempts;
} ClientData;

typedef struct {
  int mininit;
  int maxinit;
  int minfin;
  int maxfin;
  int minattempts;
  int maxattempts;
} GameData;

int acceptNewClient(int server_fd, struct sockaddr_in address,
                    ClientData **client_data, int *client_capacity, GameData *gameData);
void initializeClientData(ClientData *client_data, int start, int client_capacity);
int setupServerSocket(int port);
void handleClientActivity(int sd, ClientData *client_data,
                          struct sockaddr_in address);

#include <stdio.h>

void readData(const char *filename, int *seed, int *port, GameData *gameData) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    return;
  }
  // TODO: Check data
  fscanf(file, "%d", seed);
  fscanf(file, "%d", port);
  fscanf(file, "%d", &gameData->minattempts);
  fscanf(file, "%d", &gameData->maxattempts);
  fscanf(file, "%d", &gameData->mininit);
  fscanf(file, "%d", &gameData->maxinit);
  fscanf(file, "%d", &gameData->minfin);
  fscanf(file, "%d", &gameData->maxfin);

  fclose(file);
}

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
    printf("You can use with config file: %s <path/to/conffile.txt>\n", argv[0]);
  }else{
    readData(argv[1], &seed, &port, &gameData);
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

  server_fd = setupServerSocket(PORT);
  fcntl(server_fd, F_SETFL, O_NONBLOCK);

  printf("Listener on port %d \n", PORT);

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
      acceptNewClient(server_fd, address, &client_data, &client_capacity, &gameData);
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
                    ClientData **client_data, int *client_capacity, GameData *gameData) {
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
    valread = read(new_socket, name, BUFFER_SIZE - 1);
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

  int added = 0;
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
      (*client_data)[i].min = gameData->mininit + rand() % (gameData->maxinit - gameData->mininit + 1);
      (*client_data)[i].max = gameData->minfin + rand() % (gameData->maxfin - gameData->minfin + 1);
      (*client_data)[i].attempts = gameData->minattempts + rand() % (gameData->maxattempts - gameData->minattempts + 1);
      (*client_data)[i].secretNumber = (*client_data)[i].min + rand() % ((*client_data)[i].max - (*client_data)[i].min + 1);
      strncpy((*client_data)[i].name, name, valread);
      printf("Adding to list of sockets as %d with secret number %d, range: %d - %d\n", i, (*client_data)[i].secretNumber, (*client_data)[i].min, (*client_data)[i].max);
      added = 1;
      break;
    }
  }

  if (!added) {
    *client_capacity *= 2;
    *client_data = (ClientData *)realloc(*client_data,
                                         *client_capacity * sizeof(ClientData));
    initializeClientData(*client_data, *client_capacity / 2, *client_capacity);
    int i = *client_capacity / 2;
        (*client_data)[i].socket = new_socket;
      (*client_data)[i].min = gameData->mininit + rand() % (gameData->maxinit - gameData->mininit + 1);
      (*client_data)[i].max = gameData->minfin + rand() % (gameData->maxfin - gameData->minfin + 1);
      (*client_data)[i].attempts = gameData->minattempts + rand() % (gameData->maxattempts - gameData->minattempts + 1);
      (*client_data)[i].secretNumber = (*client_data)[i].min + rand() % ((*client_data)[i].max - (*client_data)[i].min + 1);
      strncpy((*client_data)[i].name, name, valread);
      printf("Resized client data to %d\n", *client_capacity);
      printf("Adding to list of sockets as %d with secret number %d, range: %d - %d\n", i, (*client_data)[i].secretNumber, (*client_data)[i].min, (*client_data)[i].max);
  }

  // message - min and max to user
  char *message = (char *)malloc(BUFFER_SIZE);
  sprintf(message, "h %d %d %d", (*client_data)[*client_capacity - 1].min, (*client_data)[*client_capacity - 1].max, (*client_data)[*client_capacity - 1].attempts);
  send(new_socket, message, strlen(message), 0);
  printf("User accepted\n");

  return new_socket;
}

void initializeClientData(ClientData *client_data, int start, int client_capacity) {
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
  if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
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
      printf("Client %d: %s, Secret: %d, Attempts: %d Min: %d Max: %d\n", client_data->socket, buffer,
             client_data->secretNumber, client_data->attempts, client_data->min, client_data->max);
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
      } else if(strcmp(command, "g") == 0 || strcmp(command, "l") == 0){
        send(sd, "o", 1, 0);
      }else {
        send(sd, "q", 1, 0);
      }
    } else {
      send(sd, "f", 1, 0);
    }
  }
}