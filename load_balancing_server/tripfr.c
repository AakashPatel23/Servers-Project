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
#include <unistd.h>

#define PINGS (3)
#define CPACK_LEN (7)
#define SLEEP_TIME (100000)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define M_TIME(x) ((x).tv_sec * 1000L + ((x).tv_usec / 1000))

void error(int, const char *);
void error_out(int, const char *);

void alarmcb(int signal) {
    error_out(1, "Sender is not reachable!\n");
}


int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 2, "Invalid number of arguments");

  int port = atoi(argv[1]);

  /* Sets up SIGALRM signal handler */

  struct sigaction siga;
  siga.sa_handler = alarmcb;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);
  siga.sa_flags = 0;

  error(sigaction(SIGALRM, &siga, NULL), "sigaction");

  /* Set up IP address and port for the server */

  struct sockaddr_in serverIPAddress;
  memset(&serverIPAddress, 0, sizeof(serverIPAddress));
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverIPAddress.sin_port = htons(port);

  /* Initialize server socket */

  int serverSocket = socket(PF_INET, SOCK_DGRAM, 0);
  error(serverSocket < 0, "socket");

  /* Bind server socket to IP address and port */

  error(bind(serverSocket, (struct sockaddr *)&serverIPAddress,
             sizeof(serverIPAddress)),
        "bind");

  /* Sender Initialization Verification */

  unsigned int file_size = 0;
  unsigned short block_size = 0;

  struct sockaddr_in clientIPAddress;
  int alen = sizeof(clientIPAddress);

  for (int i = 0; i < PINGS; i++) {

    char packet[CPACK_LEN] = "";

    /* Receive control packets from client */

    int num_bytes = recvfrom(serverSocket, packet, CPACK_LEN, 0,
                             (struct sockaddr *)&clientIPAddress, &alen);
    error_out(num_bytes != CPACK_LEN, "recvfrom");

    /* Examines packets */

    error_out(*packet != '(', "Control payload is not verified");

    memcpy(&file_size, packet + 1, 4);
    memcpy(&block_size, packet + 5, 2);
    file_size = ntohl(file_size);
    block_size = ntohs(block_size);
  }

  /* Send ACK control packet response */

  alarm(1);

  for (int i = 0; i < PINGS; i++) {
    char response = ')';
    sendto(serverSocket, &response, 1, 0, (struct sockaddr *)&clientIPAddress,
           alen);

    /* Sleep 100 ms */

    usleep(SLEEP_TIME);
  }

  /* Track how much time it takes to receive file */

  struct timeval start = {};

  /* Allocates memory for file */

  char *file = calloc(file_size, 1);
  error_out(!file, "calloc");
  int block_count = (file_size + block_size - 1) / block_size;
  int first = 1;

  unsigned int *datapack = calloc(block_count, sizeof(unsigned int));
  error_out(!datapack, "calloc");

  /* Allocates block for packet */

  char *block = calloc(block_size + 4, 1);
  error_out(!block, "calloc");

  while (!datapack[block_count - 1]) {
    ;
    int num_bytes = recvfrom(serverSocket, block, block_size + 4, 0,
                             (struct sockaddr *)&clientIPAddress, &alen);

    /* Extract sequence number from packet */

    uint32_t seq = 0;
    memcpy(&seq, block, 4);
    seq = ntohl(seq);

    /* Update datapack and copy to file */

    if (!datapack[seq]++) {
      if (first) {
        alarm(0);
        gettimeofday(&start, NULL);
        first = 0;
        alarm(7);
      }

      memcpy(file + (seq * block_size), block + 4, num_bytes - 4);
    }
  }

  alarm(0);
  struct timeval end = {};
  gettimeofday(&end, NULL);

  /* Write file contents to file */

  FILE *fp = fopen("received_file", "w");
  error_out(!fp, "fopen");

  fwrite(file, 1, file_size, fp);

  fclose(fp);
  fp = NULL;

  /* Post process missing packets */

  for (int i = 0; i < block_count; i++) {
    if (datapack[i] != 3) {
      printf("Packet with sequence number %d is missing %d blocks\n", i, 3 - datapack[i]);
    }
  }

  printf("\nReceiving Time: %ld ms\n", M_TIME(end) - M_TIME(start));

  free(block);
  free(file);
  close(serverSocket);
}
