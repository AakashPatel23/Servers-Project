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
#include <unistd.h>

#define PASS_LEN (5)
#define PACK_LEN (1024)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void error(int, const char *);
void error_out(int, const char *);
int invalid_password(char *);

struct nexthop {
  unsigned long dstaddr;
  unsigned short destpt;
  unsigned long srcaddr;
  unsigned short srcpt;
  unsigned short egresspt;
  int rcvudpsocket;
  int sndudpsocket;
};

struct nexthop nexthoptab[5];

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 4, "Invalid number of arguments");
  error_out(invalid_password(argv[3]), "Invalid Password");

  char *ip = argv[1];
  int port = atoi(argv[2]);
  char *pass = argv[3];

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
             sizeof(serverIPAddress)), "bind");

  /* Enter into listening mode */

  error(listen(serverSocket, 5), "listen");

  while (1) {

    /* Accept incoming connections */

    struct sockaddr_in clientIPAddress;
    int alen = sizeof(clientIPAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientIPAddress,
                              (socklen_t *)&alen);
    error(clientSocket < 0, "accept");

    /* Read request number of 200 */

    unsigned short new_request = 0;
    error(read(clientSocket, &new_request, 2) != 2, "read 200");

    if (ntohs(new_request) != 200) {
      close(clientSocket);
      continue;
    }

    /* Read secret key from client */

    char client_pass[PASS_LEN + 1] = "";
    error(read(clientSocket, client_pass, PASS_LEN) != PASS_LEN,
          "read secret key");

    if (strcmp(pass, client_pass)) {
      close(clientSocket);
      continue;
    }

    /* Assign session ID for the client */

    unsigned short sessionid = 0;
    for (; sessionid < 5; sessionid++) {
      if (!nexthoptab[sessionid].srcaddr) {
        break;
      }
    }

    if (sessionid == 5) {
      close(clientSocket);
      continue;
    }

    /* Get final destination IP and port alon with the source IP address */

    error(read(clientSocket, &nexthoptab[sessionid].dstaddr, 4) != 4,
          "read destination IP");
    nexthoptab[sessionid].dstaddr = ntohl(nexthoptab[sessionid].dstaddr);

    error(read(clientSocket, &nexthoptab[sessionid].destpt, 2) != 2,
          "read destination port");
    nexthoptab[sessionid].destpt = ntohs(nexthoptab[sessionid].destpt);

    error(read(clientSocket, &nexthoptab[sessionid].srcaddr, 4) != 4,
          "read source IP");
    nexthoptab[sessionid].srcaddr = ntohl(nexthoptab[sessionid].srcaddr);

    int k = fork();
    if (k == 0) {

      /* Child Code */
      /* Set up UDP socket for communication between source and tunneld */
      /* Binds scrpt to rcvudpsocket */

      close(serverSocket);
      struct sockaddr_in receiveIPAddress;
      memset(&receiveIPAddress, 0, sizeof(receiveIPAddress));
      receiveIPAddress.sin_family = AF_INET;
      receiveIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
      unsigned short receivePort = 56600;
      receiveIPAddress.sin_port = htons(receivePort);

      int receiveSocket = socket(PF_INET, SOCK_DGRAM, 0);
      error(receiveSocket < 0, "receive socket");

      while (bind(receiveSocket, (struct sockaddr *)&receiveIPAddress,
                  sizeof(receiveIPAddress))) {
        receivePort++;
        receiveIPAddress.sin_port = htons(receivePort);
      }

      nexthoptab[sessionid].srcpt = receivePort;
      nexthoptab[sessionid].rcvudpsocket = receiveSocket;

      /* Give the client the port that they can communicate on */

      receivePort = htons(receivePort);
      write(clientSocket, &receivePort, 2);

      /* Set up UDP socket for communication between destination and tunneld */
      /* Binds egresspt to sndudpsocket */

      struct sockaddr_in sendIPAddress;
      memset(&sendIPAddress, 0, sizeof(sendIPAddress));
      sendIPAddress.sin_family = AF_INET;
      sendIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
      unsigned short sendPort = 57500;
      sendIPAddress.sin_port = htons(sendPort);

      int sendSocket = socket(PF_INET, SOCK_DGRAM, 0);
      error(sendSocket < 0, "send socket");

      while (bind(sendSocket, (struct sockaddr *)&sendIPAddress,
                  sizeof(sendIPAddress))) {
        sendPort++;
        sendIPAddress.sin_port = htons(sendPort);
      }

      nexthoptab[sessionid].egresspt = sendPort;
      nexthoptab[sessionid].sndudpsocket = sendSocket;

      while (1) {

        /* Set up select() to monitor file descriptor activity */

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(receiveSocket, &readfds);
        FD_SET(sendSocket, &readfds);
        int maxfd = MAX(receiveSocket, sendSocket) + 1;

        error(select(maxfd, &readfds, NULL, NULL, NULL) < 0, "select");

        /* Looks for packet coming from source */

        if (FD_ISSET(receiveSocket, &readfds)) {

          /* Get packet from source and check if the address matches */

          char packet[PACK_LEN] = "";
          struct sockaddr_in sourceIPAddr;
          socklen_t sourceLen = sizeof(sourceIPAddr);
          int num_bytes =
              recvfrom(receiveSocket, packet, PACK_LEN, 0,
                       (struct sockaddr *)&sourceIPAddr, &sourceLen);
          error(num_bytes < 0, "recvfrom source");

          if (sourceIPAddr.sin_addr.s_addr !=
              htonl(nexthoptab[sessionid].srcaddr)) {
            printf("ERROR: IP Addresses do not match\n");
            continue;
          }

          nexthoptab[sessionid].srcpt = ntohs(sourceIPAddr.sin_port);

          /* Send packet to destination */

          struct sockaddr_in destIPAddr;
          memset(&destIPAddr, 0, sizeof(destIPAddr));
          destIPAddr.sin_family = AF_INET;
          destIPAddr.sin_addr.s_addr = htonl(nexthoptab[sessionid].dstaddr);
          destIPAddr.sin_port = htons(nexthoptab[sessionid].destpt);
          sendto(sendSocket, packet, num_bytes, 0,
                 (struct sockaddr *)&destIPAddr, sizeof(destIPAddr));
        }

        /* Looks for packet coming from destination */

        if (FD_ISSET(sendSocket, &readfds)) {

          /* Gets packet from destination */

          char packet[PACK_LEN] = "";
          int num_bytes = recvfrom(sendSocket, packet, PACK_LEN, 0, NULL, NULL);
          error(num_bytes < 0, "recvfrom dest");

          /* Sends packet to source */

          struct sockaddr_in sourceIPAddr;
          memset(&sourceIPAddr, 0, sizeof(sourceIPAddr));
          sourceIPAddr.sin_family = AF_INET;
          sourceIPAddr.sin_addr.s_addr = htonl(nexthoptab[sessionid].srcaddr);
          sourceIPAddr.sin_port = htons(nexthoptab[sessionid].srcpt);
          sendto(receiveSocket, packet, num_bytes, 0,
                 (struct sockaddr *)&sourceIPAddr, sizeof(sourceIPAddr));
        }
      }
    } else {

      /* Parent Code */

      close(clientSocket);
    }
  }

  close(serverSocket);
}
