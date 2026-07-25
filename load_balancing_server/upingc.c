/* Aakash Patel - pate2011 */
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PINGS (50)
#define PACK_LEN (6)
#define MICRO (1000000)

#define MICRO_TIME(i, arr) (arr[(i)].tv_sec * 1000000L + arr[(i)].tv_usec)

#define DIFF(i) (MICRO_TIME((i), pingend) - MICRO_TIME((i), pingstart))

int clientSocket;
struct timeval pingstart[PINGS] = {};
struct timeval pingend[PINGS] = {};
int responded[PINGS] = {};

void error(int, const char *);
void pingrespcb(int);
void inter_sleep(struct timespec);

/* Returns whether given password is invalid or not */

int invalid_password(char *pass) {
  int i = 0;
  while (((*pass) >= 'a' && (*pass) <= 'z') ||
         ((*pass) >= '0' && (*pass) <= '9')) {
    i++;
    pass++;
  }
  return (*pass) || (i != 4);
}

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */
  error(argc != 5, "Invalid number of arguments");
  error(invalid_password(argv[1]), "Invalid password");

  char *pass = argv[1];
  int port = atoi(argv[3]);
  char *IP = argv[2];
  int time = atoi(argv[4]);

  error(time < 1, "micsec");

  /* Sets up SIGPOLL signal handler */

  struct sigaction siga;
  siga.sa_handler = pingrespcb;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  siga.sa_flags = SA_RESTART;

  error(sigaction(SIGPOLL, &siga, NULL), "sigaction");

  /* Create and bind client socket */

  clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
  error(clientSocket < 0, "socket");

  fcntl(clientSocket, F_SETOWN, getpid());
  fcntl(clientSocket, F_SETFL, O_ASYNC);

  struct sockaddr_in clientIPAddress;
  memset(&clientIPAddress, 0, sizeof(clientIPAddress));
  clientIPAddress.sin_family = AF_INET;
  clientIPAddress.sin_port = htons(0);
  clientIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  struct sockaddr_in serverIPAddress;
  memset(&serverIPAddress, 0, sizeof(serverIPAddress));
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_port = htons(port);
  inet_pton(AF_INET, IP, &serverIPAddress.sin_addr);

  error(bind(clientSocket, (struct sockaddr *)&clientIPAddress,
             sizeof(clientIPAddress)) < 0,
        "bind");

  for (int i = 0; i < PINGS; i++) {

    /* Create UDP packet */

    char packet[PACK_LEN];
    strncpy(packet, pass, 4);
    uint16_t seq = htons(i);
    memcpy(packet + 4, &seq, 2);

    gettimeofday(&pingstart[i], NULL);
    sendto(clientSocket, packet, PACK_LEN, 0,
           (struct sockaddr *)&serverIPAddress, sizeof(serverIPAddress));

    /* Sleep between pings */

    if (i != PINGS - 1) {
      struct timespec ts;
      ts.tv_sec = time / MICRO;
      ts.tv_nsec = (time % MICRO) * 1000;
      inter_sleep(ts);
    }
  }

  /* sleep 750 ms after all pings are done */

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 750000000;
  inter_sleep(ts);
  close(clientSocket);

  long max = 0;
  long average = 0;
  int count = 0;
  long min = 0;

  /* Calculate min, avg, and max of diffs */

  for (int i = 0; i < PINGS; i++) {
    long diff = DIFF(i);
    if (diff <= MICRO || responded[i]) {
      if (!count || diff > max) {
        max = diff;
      }
      if (!count || diff < min) {
        min = diff;
      }
      average += diff;
      count++;
    }
  }

  if (count) {
    average /= count;
    printf(
        "Min: %ld microseconds\nAvg: %ld microseconds\nMax: %ld microseconds\n",
        min, average, max);
  }
}
