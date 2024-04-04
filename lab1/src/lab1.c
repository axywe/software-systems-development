#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void pipe_exec(char *cmd[], int input_fd, int output_fd) {
  if (fork() == 0) {
    if (input_fd != STDIN_FILENO) {
      dup2(input_fd, STDIN_FILENO);
      close(input_fd);
    }
    if (output_fd != STDOUT_FILENO) {
      dup2(output_fd, STDOUT_FILENO);
      close(output_fd);
    }
    execvp(cmd[0], cmd);
    fprintf(stderr, "Error starting %s\n", cmd[0]);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <archive> \"<pattern>\"\n", argv[0]);
    return 1;
  }

  int pipe1[2];

  if (pipe(pipe1) == -1) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  char *ar_cmd[] = {"ar", "-t", argv[1], NULL};
  char *grep_cmd[] = {"grep", "-e", argv[2], NULL};

  pipe_exec(ar_cmd, STDIN_FILENO, pipe1[1]);
  close(pipe1[1]);

  pipe_exec(grep_cmd, pipe1[0], STDOUT_FILENO);
  close(pipe1[0]);

  while (wait(NULL) > 0)
    ;

  return 0;
}
