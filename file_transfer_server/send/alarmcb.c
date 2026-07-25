#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

/* Handles expiring alarm */

void error_out(int, const char *);

void alarmcb(int signal) {
  error_out(1, "Receiver is not reachable!\n");
}
