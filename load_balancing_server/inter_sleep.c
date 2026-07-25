/* Aakash Patel - pate2011 */
#include <time.h>

/* Makes sure a nanosleep session that is interrupted sleeps */

void inter_sleep(struct timespec ts) {
  struct timespec leftover;
  while (nanosleep(&ts, &leftover)) {
    ts = leftover;
  }
}
