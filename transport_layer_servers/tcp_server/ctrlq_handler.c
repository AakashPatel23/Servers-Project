#include <unistd.h>

/* Prints Ctrl-\ message when pressed */

void ctrlq_handler(int signal) {
  write(1, "\nUse 'q' to quit.\n% ", 20);
}
