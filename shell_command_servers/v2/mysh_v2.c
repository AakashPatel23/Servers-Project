#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARG_LEN (32)

int main(void) {
  int k;
  char buf[20];
  int status;
  int len;

  while (1) {

    /* print shell prompt */
    fprintf(stdout, "%d $ ", getpid());

    /* read command from stdin */
    /* stops when newline is entered or 19 (n - 1) bytes are read */
    fgets(buf, 20, stdin);
    len = strlen(buf);

    if (len == 1) /* case: only enter/return key pressed */
      continue;
    buf[len - 1] = '\0'; /* case: command entered */

    char *args[ARG_LEN] = {};
    int num_args = 0;
    char *ptr_to_buf = buf;

    /* skip spaces */
    while (*ptr_to_buf == ' ') {
      ptr_to_buf++;
    }

    /* keep track of beginning of a word */
    char *start = ptr_to_buf;

    while (*ptr_to_buf) {
      /* find and terminate end of a word */
      if (*ptr_to_buf == ' ') {
        *ptr_to_buf = '\0';
        args[num_args] = start;
        num_args++;
        ptr_to_buf++;
        while (*ptr_to_buf == ' ') {
          ptr_to_buf++;
        }
        start = ptr_to_buf;
      } else {
        ptr_to_buf++;
      }
    }

    /* if word is empty, don't count it as an argument */
    if (*start) {
      args[num_args] = start;
      args[num_args + 1] = NULL;
    } else {
      args[num_args] = NULL;
    }

    if (execvp(args[0], args) == -1) // if execution failed, terminate child
      exit(1);
  }
}
