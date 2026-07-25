/* Aakash Patel - pate2011 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Handles Error Checking */

void error_out(int condition, const char * error_message) {
  if (condition) {
    printf("%s\n", error_message);
    exit(1);
  }
}
