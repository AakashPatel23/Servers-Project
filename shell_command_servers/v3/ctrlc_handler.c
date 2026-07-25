#include <unistd.h>
#include <stdio.h>

/* Prints Ctrl-c message when pressed */

void ctrlc_handler(int signal) {
  write(1, "\nUse 'q' to quit.\n", 18);
  fprintf(stdout, "%d $ ", getpid());
  fflush(stdout);
}
