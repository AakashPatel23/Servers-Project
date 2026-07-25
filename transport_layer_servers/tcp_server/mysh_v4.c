// Concurrent server example: simple shell using fork() and execlp().

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LEN (128)

int invalid_password(char *);
void error(int, const char *);

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error(argc != 3, "Invalid number of arguments");
  error(invalid_password(argv[1]), "Invalid password");

  char *pass = argv[1];
  int port = atoi(argv[2]);

  /* Set up IP address and port for the server */

  struct sockaddr_in serverIPAddress;
  memset(&serverIPAddress, 0, sizeof(serverIPAddress));
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverIPAddress.sin_port = htons(port);

  /* Initialize server socket */

  int masterSocket = socket(PF_INET, SOCK_STREAM, 0);
  error(masterSocket < 0, "socket");

  /* Set up socket to reuse ports */

  int optval = 1;
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
                       sizeof(int));

  /* Bind server socket to IP address and port */

  error(bind(masterSocket, (struct sockaddr *)&serverIPAddress,
             sizeof(serverIPAddress)),
        "bind");

  /* Enter into listening mode */

  error(listen(masterSocket, 5), "listen");

  int k;
  int status;
  while (1) {

    /* Accept incoming connections */

    struct sockaddr_in clientIPAddress;
    int alen = sizeof(clientIPAddress);
    int slaveSocket = accept(masterSocket, (struct sockaddr *)&clientIPAddress, (socklen_t *)&alen);
    error(slaveSocket < 0, "accept");

    /* Read client request */

    char request[LEN];
    char c = '\0';
    int num_bytes = 0;

    while ((read(slaveSocket, &c, 1) > 0) && (c != '\n') && (num_bytes < LEN - 1)) {
      request[num_bytes] = c;
      num_bytes++;
    }
    request[num_bytes] = '\0';

    /* Check if request was empty */

    if (!num_bytes) {
      close(slaveSocket);
      continue;
    }

    /* Split up the password and command from request and check password */

    char client_pass[5] = "";
    strncpy(client_pass, request, 4);
    client_pass[4] = '\0';
    char *cmd = request;
    cmd += 5;

    if (strcmp(pass, client_pass)) {
      printf("Incorrect secret key from %s: %d, client request: %s\n",
             inet_ntoa(clientIPAddress.sin_addr),
             ntohs(clientIPAddress.sin_port), cmd);
      close(slaveSocket);
      continue;
    }

    /* Exits server given quit command */

      if (!strcmp(cmd, "q")) {
        close(slaveSocket);
        close(masterSocket);
        exit(0);
      }

      char *args[LEN] = {};
      int num_args = 0;

      /* Skip spaces */

      while (*cmd == ' ') {
        cmd++;
      }

      /* Keep track of beginning of a word */

      char *start = cmd;

      while (*cmd) {

        /* Find and terminate end of a word */

        if (*cmd == ' ') {
          *cmd = '\0';
          args[num_args] = start;
          num_args++;
          cmd++;
          while (*cmd == ' ') {
            cmd++;
          }
          start = cmd;
        } else {
          cmd++;
        }
      }

      /* If word is empty, don't count it as an argument */

      if (*start) {
        args[num_args] = start;
        args[num_args + 1] = NULL;
      } else {
        args[num_args] = NULL;
      }

      k = fork();
      if (k == 0) {

        /* Child code */

        close(masterSocket);
        dup2(slaveSocket, 1);
        dup2(slaveSocket, 2);
        close(slaveSocket);
        if (execvp(args[0], args) == -1) {
          exit(1);
        }
      } else {

        /* Parent code (blocks until child terminates) */

        close(slaveSocket);
        waitpid(k, &status, 0);
      }
    }
  }
