// Include necessary headers: standard I/O, standard lib, pthreads for
// threading, unistd for various constants, and sys/time for measuring execution
// time
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
// Define constants for the diffusion equation parameters and boundary
// conditions
#define COEF 1
#define dx 2
#define dy 2
#define MTIME 50
#define TOP 0.6
#define BOTTOM 20
#define LEFT 0.4
#define RIGHT 40
//#define WRITE_IN_FILE

// Define a structure for thread parameters, including thread ID, grid
// dimensions, time step, and indexes for the portion of the grid each thread is
// responsible for
typedef struct {
  pthread_t tid;
  int n, m;
  double dt;
  int firstIndexStart, firstIndexEnd, secondIndexStart, secondIndexEnd;
} Thread_param;

// Initialize a mutex and a barrier for thread synchronization
pthread_mutex_t mutx;
pthread_barrier_t barr;
// Declare pointers for storing the previous and current states of the
// temperature grid
Thread_param *threads;
double *prevLayer, *currLayer;

// Optionally include a function to write the grid state to a file, used if
// WRITE_IN_FILE is defined
#ifdef WRITE_IN_FILE
void into_file(FILE *output, double *layer, int N, int M) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < M; j++)
      fprintf(output, "%d %d %lf\n", i, j, layer[N * j + i]);
  fprintf(output, "\n\n");
}
#endif

// Function to calculate boundary conditions based on position in the grid
double boundary(int i, int j, int n, int m, double dt, double T) {
  double grad_top = 1.0;  // Градиент на верхней границе
  double grad_left = 1.0; // Градиент на левой границе
  if (i == 0)
    return T + grad_top * dx;
  else if (i == n - 1)
    return BOTTOM;
  else if (j == 0)
    return T + grad_left * dy;
  else if (j == m - 1)
    return RIGHT;
  return T; // Should never reach here.
}

// Main function executed by each thread to solve the heat equation over its
// part of the grid
void *solver(void *arg_p) {
  Thread_param *param = (Thread_param *)arg_p;
  for (double t = 0.0 + param->dt; t <= MTIME; t += param->dt) {
    pthread_barrier_wait(&barr);
    for (int i = param->firstIndexStart; i <= param->firstIndexEnd; i++)
      for (int j = param->secondIndexStart; j <= param->secondIndexEnd; j++) {
        if ((i != 0) && (i != param->n - 1) && (j != 0) &&
            (j != param->m - 1)) {
          double x1 = (prevLayer[param->n * j + i - 1] -
                       2 * prevLayer[param->n * j + i] +
                       prevLayer[param->n * j + i + 1]) /
                      (dx * dx);
          double x2 = (prevLayer[param->n * (j - 1) + i] -
                       2 * prevLayer[param->n * j + i] +
                       prevLayer[param->n * (j + 1) + i]) /
                      (dy * dy);
          currLayer[param->n * j + i] =
              param->dt * COEF * (x1 + x2) + prevLayer[param->n * j + i];
        } else {
          currLayer[param->n * j + i] = boundary(
              i, j, param->n, param->m, param->dt, prevLayer[param->n * j + i]);
        }
      }
    pthread_barrier_wait(&barr);
#ifdef WRITE_IN_FILE
    if (pthread_mutex_trylock(&mutx) == 0) {
      double *interm = prevLayer;
      prevLayer = currLayer;
      currLayer = interm;
      FILE *output = fopen("out", "a");
      into_file(output, currLayer, param->n, param->m);
      fclose(output);
      pthread_mutex_unlock(&mutx);
    }
#endif
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  // Check for valid command-line arguments and handle various constraints and
  // errors
  if (argc != 5) {
    printf("Invalid argc\n");
    return -1;
  }
  unsigned int value = (1U << 30) - 2;
  if ((atoi(argv[3]) * atoi(argv[4])) > value) {
    printf("Too many nodes\n");
    return -3;
  }
  // Parse command-line arguments for the number of threads, time step, and grid
  // dimensions
  int count = atoi(argv[1]), N = atoi(argv[3]) + 2, M = atoi(argv[4]) + 2;
  double dt = atof(argv[2]);
  // Record start time for measuring execution time
  struct timeval start, end;
  gettimeofday(&start, NULL);
  // Allocate memory for storing the grid states
  prevLayer = calloc(N * M, sizeof(double));
  currLayer = calloc(N * M, sizeof(double));
  // Initialize pthread attributes, mutex, and barrier
  pthread_attr_t attr;
  pthread_mutex_init(&mutx, NULL);
  pthread_barrier_init(&barr, NULL, count);
  // Allocate memory for thread parameters and configure each thread's part of
  // the grid
  threads = calloc(count, sizeof(Thread_param));
  // Initialize the grid with boundary conditions
  for (int i = 0; i < count; i++) {
    threads[i] = (Thread_param){.n = N,
                                .m = M,
                                .dt = dt,
                                .firstIndexStart = 1 + i * ((N - 2) / count),
                                .firstIndexEnd = (i + 1) * ((N - 2) / count),
                                .secondIndexStart = 0,
                                .secondIndexEnd = M - 1};
    if (i == 0)
      threads[i].firstIndexStart--;
    if (i == count - 1)
      threads[i].firstIndexEnd++;
  }
  for (int i = 0; i < N; i++)
    for (int j = 0; j < M; j++)
      prevLayer[N * j + i] = (i == 0 || i == N - 1 || j == 0 || j == M - 1)
                                 ? boundary(i, j, N, M, dt, 0.01)
                                 : 0;
      // Optionally write the initial grid state to a file
#ifdef WRITE_IN_FILE
  FILE *output = fopen("out", "w");
  into_file(output, prevLayer, N, M);
  fclose(output);
#endif
  // Set pthread attributes for system-wide contention scope and joinable state
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // Create threads to start solving the heat equation
  for (int i = 0; i < count; i++)
    pthread_create(&threads[i].tid, &attr, solver, &threads[i]);
  // Join threads after completion
  for (int i = 0; i < count; i++)
    pthread_join(threads[i].tid, NULL);
  // Clean up: destroy mutex and barrier, print execution time, and free
  // allocated memory
  pthread_mutex_destroy(&mutx);
  pthread_barrier_destroy(&barr);
  gettimeofday(&end, NULL);
  long seconds = (end.tv_sec - start.tv_sec);
  long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
  printf("Execution time: %ld seconds, %ld microseconds\n", seconds, micros);
  // Optionally write a configuration file for gnuplot to visualize the results
#ifdef WRITE_IN_FILE
  FILE *fp = fopen("gnuplot.cfg", "w");
  if (fp) {
    char *message;
    fprintf(fp,
            "set term gif animate\nset output 'animation.gif'\nset zrange "
            "[0:50]\nset dgrid3d\nset hidden3d\ndo for [i=0:%d] {\nsplot 'out' "
            "index i using 1:2:3 with lines\n}",
            (int)(MTIME / dt) + 1);
    fclose(fp);
    printf("gnuplot -persist gnuplot.cfg\n");
  } else {
    printf("Error recording gnuplot config\n");
  }
#endif
  free(prevLayer);
  free(currLayer);
  free(threads);
  return 0;
}