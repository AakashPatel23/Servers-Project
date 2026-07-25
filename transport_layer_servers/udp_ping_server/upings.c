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

#define PACK_LEN (6)

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

  int serverSocket = socket(PF_INET, SOCK_DGRAM, 0);
  error(serverSocket < 0, "socket");

  /* Bind server socket to IP address and port */

  error(bind(serverSocket, (struct sockaddr *)&serverIPAddress,
             sizeof(serverIPAddress)),
        "bind");

  while (1) {
    char packet[PACK_LEN] = "";

    /* Receive packets from client */

    struct sockaddr_in clientIPAddress;
    int alen = sizeof(clientIPAddress);
    int num_bytes = recvfrom(serverSocket, packet, PACK_LEN, 0,
                             (struct sockaddr *)&clientIPAddress, &alen);
    if (num_bytes == PACK_LEN) {

      /* Check password and sequence number */

      if (memcmp(packet, pass, 4)) {
        printf("Incorrect secret key from %s: %d\n",
               inet_ntoa(clientIPAddress.sin_addr),
               ntohs(clientIPAddress.sin_port));
        continue;
      }

      /* Send packets back to client */

      sendto(serverSocket, packet, PACK_LEN, 0,
             (struct sockaddr *)&clientIPAddress, alen);
    }
  }
  close(serverSocket);
}
