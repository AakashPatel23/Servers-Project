#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define PACK_LEN (6)
#define PINGS (50)

/* Handles receiving of a packet */

extern int clientSocket;
extern struct timeval pingend[PINGS];
extern int responded[PINGS];

void pingrespcb(int signal) {
  char packet[PACK_LEN] = "";
  recvfrom(clientSocket, packet, PACK_LEN, 0, NULL, NULL);

  uint16_t seq;
  memcpy(&seq, packet + 4, 2);
  seq = ntohs(seq);

  if (seq < PINGS) {
    gettimeofday(&pingend[seq], NULL);
    responded[seq] = 1;
  }
}
