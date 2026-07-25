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

#define PASS_LEN (5)

void error(int, const char *);
void error_out(int, const char *);
int invalid_password(char *);

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 7, "Invalid number of arguments");
  error_out(invalid_password(argv[3]), "Invalid Password");

  /* Populate command-line input */

  char *tunneldIP = argv[1];
  unsigned short tunneldPort = atoi(argv[2]);
  char *pass = argv[3];
  char *sourceAddr = argv[4];
  char *destAddr = argv[5];
  unsigned short destPort = htons(atoi(argv[6]));

  /* Create and bind client socket */

  int clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  error(clientSocket < 0, "socket");

  struct sockaddr_in tunneldIPAddress;
  memset(&tunneldIPAddress, 0, sizeof(tunneldIPAddress));
  tunneldIPAddress.sin_family = AF_INET;
  tunneldIPAddress.sin_port = htons(tunneldPort);
  inet_pton(AF_INET, tunneldIP, &tunneldIPAddress.sin_addr);

  error(connect(clientSocket, (struct sockaddr *)&tunneldIPAddress,
                sizeof(tunneldIPAddress)) < 0,
        "connect");

  /* Send 200, secret key, destination IP address + port, and source IP address  to tunneld */

  unsigned short request = htons(200);
  write(clientSocket, &request, 2);
  write(clientSocket, pass, PASS_LEN);

  uint32_t destIP = 0;
  inet_pton(AF_INET, destAddr, &destIP);
  write(clientSocket, &destIP, 4);
  write(clientSocket, &destPort, 2);

  uint32_t sourceIP = 0;
  inet_pton(AF_INET, sourceAddr, &sourceIP);
  write(clientSocket, &sourceIP, 4);

  /* Get receive port from tunneld */

  unsigned short port = 0;
  read(clientSocket, &port, 2);
  port = ntohs(port);
  printf("Port Number: %d\n", port);
  close(clientSocket);
}
