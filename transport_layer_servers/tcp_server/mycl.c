// Concurrent server example: simple shell using fork() and execlp().

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LEN (64)
#define REQUEST_LEN (128)
#define OUTPUT_LEN (1024)

void ctrlc_handler(int);
void ctrlq_handler(int);
int invalid_password(char *);
void error(int, const char *);

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */
  error(argc != 4, "Invalid number of arguments");
  error(invalid_password(argv[1]), "Invalid password");

  char *pass = argv[1];
  int port = atoi(argv[3]);
  char *IP = argv[2];

  char command[LEN];
  char output[OUTPUT_LEN];
  int len;

  /* Sets up SIGINT signal handler */

  struct sigaction siga;
  siga.sa_handler = ctrlc_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  siga.sa_flags = SA_RESTART;

  error(sigaction(SIGINT, &siga, NULL), "sigaction");

  /* Sets up SIGQUIT signal handler */

  struct sigaction sigaq;
  sigaq.sa_handler = ctrlq_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&sigaq.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  sigaq.sa_flags = SA_RESTART;

  error(sigaction(SIGQUIT, &sigaq, NULL), "sigaction");

  while (1) {

    /* Create client socket */

    int clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    error(clientSocket < 0, "socket");

    struct sockaddr_in serverIPAddress;
    memset(&serverIPAddress, 0, sizeof(serverIPAddress));
    serverIPAddress.sin_family = AF_INET;
    serverIPAddress.sin_port = htons(port);
    inet_pton(AF_INET, IP, &serverIPAddress.sin_addr);

    error(connect(clientSocket, (struct sockaddr *)&serverIPAddress,
                  sizeof(serverIPAddress)) < 0, "connect");

    /* Print prompt and read command */

    fprintf(stdout, "%% ");

    fgets(command, LEN, stdin);
    len = strlen(command);
    if (len == 1)
      continue;
    command[len - 1] = '\0';
    char *cmd = command;

    /* Gets rid of leading spaces */

    while (*cmd == ' ') {
      cmd++;
    }

    /* Write to server and quit if needed */

    char request[REQUEST_LEN] = "";
    snprintf(request, REQUEST_LEN, "%s %s\n", pass, cmd);
    write(clientSocket, request, strlen(request));
    if (!strcmp(cmd, "q")) {
      close(clientSocket);
      exit(0);
    }

    /* Reads server output */

    int num_bytes = 0;
    char c = '\0';
    while (read(clientSocket, &c, 1) > 0 && num_bytes < OUTPUT_LEN - 1) {
      output[num_bytes] = c;
      num_bytes++;
    }
    output[num_bytes] = '\0';

    /* Print output to terminal */

    fprintf(stdout, "%s", output);

    close(clientSocket);
  }
}
