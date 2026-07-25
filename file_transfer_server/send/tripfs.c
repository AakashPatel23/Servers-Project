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

#define PINGS (3)
#define CPACK_LEN (7)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define M_TIME(x) ((x).tv_sec * 1000L + ((x).tv_usec / 1000))

int invalid_filename(char *);
void error(int, const char *);
void error_out(int, const char *);
void alarmcb(int);
int dropornot(int);

int packselect[5][2];

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 6, "Invalid number of arguments");
  error_out(invalid_filename(argv[1]), "Invalid filename");

  /* Populate command-line input */

  char *filename = argv[1];
  char *ip = argv[2];
  unsigned int port = atoi(argv[3]);
  unsigned int mic_time = atoi(argv[4]);
  unsigned short block_size = atoi(argv[5]);

  error_out(!block_size, "blocksz");

  /* Sets up SIGALRM signal handler */

  struct sigaction siga;
  siga.sa_handler = alarmcb;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);
  siga.sa_flags = 0;

  error(sigaction(SIGALRM, &siga, NULL), "sigaction");

  /* Create and bind client socket */

  int clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
  error(clientSocket < 0, "socket");

  struct sockaddr_in clientIPAddress;
  memset(&clientIPAddress, 0, sizeof(clientIPAddress));
  clientIPAddress.sin_family = AF_INET;
  clientIPAddress.sin_port = htons(0);
  clientIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  struct sockaddr_in serverIPAddress;
  memset(&serverIPAddress, 0, sizeof(serverIPAddress));
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_port = htons(port);
  inet_pton(AF_INET, ip, &serverIPAddress.sin_addr);

  error(bind(clientSocket, (struct sockaddr *)&clientIPAddress,
             sizeof(clientIPAddress)) < 0,
        "bind");

  /* Open param.dat and populate packselect */

  FILE *fp = fopen("param.dat", "r");
  error_out(!fp, "fopen");

  int num = 0;

  fscanf(fp, "%d", &num);

  for (int i = 0; i < num; i++) {
    fscanf(fp, "%d %d", packselect[i], packselect[i] + 1);
  }

  fclose(fp);

  /* Open and read file */

  fp = fopen(filename, "r");
  error_out(!fp, "fopen");

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  rewind(fp);

  char *file = calloc(file_size, 1);
  error_out(!file, "calloc");

  fread(file, 1, file_size, fp);

  fclose(fp);
  fp = NULL;

  /* Send control packets */

  for (int i = 0; i < PINGS; i++) {

    /* Create packet */

    char packet[CPACK_LEN];
    packet[0] = '(';
    uint32_t fsize = htonl(file_size);
    uint16_t bsize = htons(block_size);
    memcpy(packet + 1, &fsize, 4);
    memcpy(packet + 5, &bsize, 2);

    sendto(clientSocket, packet, CPACK_LEN, 0,
           (struct sockaddr *)&serverIPAddress, sizeof(serverIPAddress));

    /* Sleep between pings */

    if (mic_time) {
      usleep(mic_time);
    }
  }

  /* Set 3 second alarm and receive response, if timer doesn't go off cancel by
   * calling it with argument 0 */

  alarm(3);

  char response;
  recvfrom(clientSocket, &response, 1, 0, NULL, NULL);
  error_out(response != ')', "Response is not verified");

  alarm(0);

  /* Track how much time it takes to send file */

  struct timeval start = {};
  gettimeofday(&start, NULL);

  /* Send file blocks to receiver */

  unsigned int block_count = 0;
  long file_left = file_size;
  char *file_ptr = file;

  /* Allocates block for packet */

  char *block = calloc(block_size + 4, 1);
  error_out(!block, "calloc");

  while (file_left) {

    /* Populates packet with sequence number and file contents */

    int size = MIN(file_left, block_size);
    uint32_t seq = htonl(block_count);
    memcpy(block, &seq, 4);
    memcpy(block + 4, file_ptr, size);

    /* Sends 3 duplicated packets */

    for (int i = 0; i < PINGS; i++) {
      if (!dropornot(block_count)) {
        sendto(clientSocket, block, size + 4, 0,
               (struct sockaddr *)&serverIPAddress, sizeof(serverIPAddress));
      }
      if (mic_time) {
        usleep(mic_time);
      }
    }

    file_ptr += size;
    file_left -= size;
    block_count++;
  }

  struct timeval end = {};
  gettimeofday(&end, NULL);

  printf("Transmission Time: %ld ms\n", M_TIME(end) - M_TIME(start));

  free(block);
  free(file);
  close(clientSocket);
}
