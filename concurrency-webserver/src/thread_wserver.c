#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include "common_threads.h"

char default_root[] = ".";

int* conn_buffer;
int use_ptr = 0;

int num_workers = 10;

int fill = 0;
int use = 0;

int BUFFER_SIZE = 20;

sem_t queue_empty;

sem_t buffer_sem;
sem_t work_queue;
sem_t pending;

void *request_handler(void *arg) {
  int conn_fd = (int) arg;
  printf("conn_fd: %d\n", conn_fd);
  request_handle(conn_fd);
  close_or_die(conn_fd);
  return NULL;
}

void do_fill(int conn_fd) {
  conn_buffer[fill] = conn_fd;
  fill++;
  printf("do_fill: fill: %d\n", fill);
  if (fill == BUFFER_SIZE) {
    fill = 0;
  }
}

int do_get() {
  int tmp = conn_buffer[use];
  use++;
  printf("do_get: use: %d\n", use);
  if (use == BUFFER_SIZE) {
    use = 0;
  }
  return tmp;
}

void *handle(void* arg) {
  int tid = (int) arg;
  while (1) {
    printf("handle: thread %d waiting for work queue\n", tid);
    sem_wait(&work_queue);
    int conn_fd;
    printf("handle: thread %d waiting for buffer lock\n", tid);
    sem_wait(&buffer_sem);
    conn_fd = do_get();
    sem_post(&buffer_sem);

    request_handle(conn_fd);
    close_or_die(conn_fd);
    sem_post(&queue_empty);
  }
}

void *accept_request(void *arg) {
  int listen_fd = (int) arg;
  printf("accept_request: listen_fd: %d\n", listen_fd);
  while (1) {
    sem_wait(&queue_empty);
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
    printf("accept_request: conn_fd: %d\n", conn_fd);

    sem_wait(&buffer_sem);
    do_fill(conn_fd);
    sem_post(&work_queue);
    printf("done waking work queue\n");
    sem_post(&buffer_sem);
  }
  return NULL;
}

//
// ./thread_wserver [-d <basedir>] [-p <portnum>]  [-t <number of threads>]
//
int main(int argc, char *argv[]) {
  int c;
  char *root_dir = default_root;
  int port = 10000;
  int threads = 1;
  while ((c = getopt(argc, argv, "d:p:t:")) != -1)
    switch (c) {
    case 'd':
	    root_dir = optarg;
	    break;
    case 'p':
	    port = atoi(optarg);
	    break;
    case 't':
      threads = atoi(optarg);
      break;
    default:
	    fprintf(stderr, "./thread_wserver [-d <basedir>] [-p <portnum>]  [-t <number of threads>]");
	    exit(1);
    }

  // run out of this directory
  chdir_or_die(root_dir);

  conn_buffer = (int*)malloc(BUFFER_SIZE * sizeof(int));
  assert(conn_buffer != NULL);
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    conn_buffer[i] = 0;
  }

  sem_init(&queue_empty, 0, 1);
  sem_init(&buffer_sem, 0, 1);
  sem_init(&work_queue, 0, 0);
  sem_init(&pending, 0, 1);

  // now, get to work
  int listen_fd = open_listen_fd_or_die(port);
  printf("main: listen_fd: %d\n", listen_fd);


  pthread_t master;
  Pthread_create(&master, NULL, accept_request, (void*) listen_fd);

  pthread_t workers[num_workers];
  for (int i = 0; i < num_workers; i++) {
    Pthread_create(&workers[i], NULL, handle, (void*) i);
  }

  for (int i = 0; i < num_workers; i++) {
    Pthread_join(workers[i], NULL);
  }
  Pthread_join(master, NULL);
  return 0;
}
