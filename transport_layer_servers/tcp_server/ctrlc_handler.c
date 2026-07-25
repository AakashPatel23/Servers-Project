#include <unistd.h>

/* Prints Ctrl-c message when pressed */

void ctrlc_handler(int signal) {
  write(1, "\nUse 'q' to quit.\n% ", 20);
}
