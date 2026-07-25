/* Aakash Patel - pate2011 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Handles Error Checking */

void error(int condition, const char * error_message) {
  if (condition) {
    perror(error_message);
    exit(1);
  }
}
