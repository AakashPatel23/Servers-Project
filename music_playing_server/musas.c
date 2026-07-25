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

#define FILE_LEN (16)
#define LOG_FILE_LEN (64)
#define LOG_LEN (4096)
#define BUFSIZE (4096)
#define NANO (1000000000L)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MICRO_TIME(i) (((i).tv_sec * 1000000L) + (i).tv_usec)


void error(int, const char *);
void error_out(int, const char *);
int invalid_filename(char *);

int active_sessions;
int session_counter;
int UDPSocket;
float ilambda_log[LOG_LEN];

/* Receives packet from client */

void rcvpacket(int signal) {
  float ilambda = 0.0;
  recvfrom(UDPSocket, &ilambda, sizeof(float), 0, NULL, NULL);
  ilambda_log[0] = ilambda;
}

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 5, "Invalid number of arguments");

  char *ip = argv[1];
  unsigned short port = atoi(argv[2]);
  float ilambda = atof(argv[3]);
  char *logfile = argv[4];

  /* Sets up SIGPOLL signal handler */

  struct sigaction siga;
  siga.sa_handler = rcvpacket;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  siga.sa_flags = SA_RESTART;

  error(sigaction(SIGPOLL, &siga, NULL), "sigaction");

  /* Set up IP address and port for the server's TCP connection */

  struct sockaddr_in serverIPAddress;
  memset(&serverIPAddress, 0, sizeof(serverIPAddress));
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_port = htons(port);
  inet_pton(AF_INET, ip, &serverIPAddress.sin_addr);

  /* Initialize server TCP socket */

  int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
  error(serverSocket < 0, "server socket");

  int opt = 1;
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  /* Bind server socket to IP address and port */

  error(bind(serverSocket, (struct sockaddr *)&serverIPAddress,
             sizeof(serverIPAddress)),
        "bind");

  /* Enter into listening mode */

  error(listen(serverSocket, 2), "listen");

  while (1) {

    while (waitpid(-1, NULL, WNOHANG) > 0) {
      active_sessions--;
    }

    /* Accept incoming connections */

    struct sockaddr_in clientIPAddress;
    int alen = sizeof(clientIPAddress);
    int TCPSocket = accept(serverSocket, (struct sockaddr *)&clientIPAddress,
                           (socklen_t *)&alen);
    error(TCPSocket < 0, "accept");

    /* Read audio file name and check its validity */

    char file[FILE_LEN] = "";
    int num_bytes = 0;
    while (read(TCPSocket, &file[num_bytes], 1) > 0 && num_bytes < FILE_LEN) {
      if (!file[num_bytes]) {
        break;
      }
      num_bytes++;
    }

    if (invalid_filename(file) || active_sessions >= 2) {
      write(TCPSocket, "q", 1);
      close(TCPSocket);
      continue;
    }

    session_counter++;

    /* Fork a child that handles audio streaming */

    int k = fork();
    if (k == 0) {

      /* Child Code */

      printf("New streaming session has been accepted: file = %s, client IP = "
             "%s, client Port = %d\n",
             file, inet_ntoa(clientIPAddress.sin_addr),
             ntohs(clientIPAddress.sin_port));
      close(serverSocket);

      /* Set up UDP socket for communication */

      struct sockaddr_in IPAddress;
      memset(&IPAddress, 0, sizeof(IPAddress));
      IPAddress.sin_family = AF_INET;
      IPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
      IPAddress.sin_port = htons(0);

      UDPSocket = socket(PF_INET, SOCK_DGRAM, 0);
      error(UDPSocket < 0, "UDPSocket");

      error(bind(UDPSocket, (struct sockaddr *)&IPAddress, sizeof(IPAddress)) < 0,
            "bind UDP");

      unsigned short clientPort = 0;
      error(read(TCPSocket, &clientPort, 2) != 2, "read client port");

      clientIPAddress.sin_port = clientPort;


      /* Open audio file */

      FILE *audiofile = fopen(file, "rb");
      error(!audiofile, "fopen audiofile");

      struct timeval timestamps[LOG_LEN] = {0};
      int num_entries = 0;

      gettimeofday(timestamps, NULL);
      *ilambda_log = ilambda;

      fcntl(UDPSocket, F_SETOWN, getpid());
      fcntl(UDPSocket, F_SETFL, O_ASYNC | O_NONBLOCK);

      char packet[BUFSIZE] = "";
      num_bytes = 0;

      /* Read audio file and sensd to client */

      while ((num_bytes = fread(packet, 1, BUFSIZE, audiofile)) > 0) {

        sendto(UDPSocket, packet, num_bytes, 0,
               (struct sockaddr *)&clientIPAddress, sizeof(clientIPAddress));

        if (num_entries < LOG_LEN - 1) {
          num_entries++;
          gettimeofday(timestamps + num_entries, NULL);
          ilambda_log[num_entries] = ilambda_log[0];
        }

        struct timespec sleep;
        sleep.tv_sec = (int) ilambda_log[0];
        sleep.tv_nsec = ((long) (ilambda_log[0] * NANO)) % NANO;
        nanosleep(&sleep, NULL);

      }

      fclose(audiofile);
      audiofile = NULL;


      write(TCPSocket, "Q", 1);
      close(TCPSocket);

      /* Write to log file */

      char new_logfile[LOG_FILE_LEN] = "";
      snprintf(new_logfile, LOG_FILE_LEN, "%d%s", session_counter, logfile);
      FILE *fp = fopen(new_logfile, "w");
      error(!fp, "fopen logfile");
      long start_time = MICRO_TIME(timestamps[0]);
      ilambda_log[0] = ilambda;
      for (int i = 0; i <= num_entries; i++) {
        fprintf(fp, "ilambda at Time %.3f: %f\n", (MICRO_TIME(timestamps[i]) - start_time) / 1000.0, ilambda_log[i]);
      }
      fclose(fp);
      fp = NULL;

      close(UDPSocket);
      exit(0);

    } else {

      /* Parent Code */

      active_sessions++;
      write(TCPSocket, "K", 1);
      close(TCPSocket);
    }
  }

  close(serverSocket);
}
