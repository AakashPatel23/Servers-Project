// Concurrent server example: simple shell using fork() and execlp().

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void ctrlc_handler(int);

void ctrlq_handler(int);

int main(void) {
  int k;
  char buf[20];
  int status;
  int len;

  /* Sets up SIGINT signal handler */
  struct sigaction siga;
  siga.sa_handler = ctrlc_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while
   * ctrlc_handler runs */
  sigemptyset(&siga.sa_mask);

  /* SA_RESTART will continue doing syscalls after ctrlc_handler is finished */
  siga.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &siga, NULL)) {
    perror("sigaction");
    exit(1);
  }

  /* Sets up SIGQUIT signal handler */
  struct sigaction sigaq;
  sigaq.sa_handler = ctrlq_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while
   * ctrlq_handler runs */
  sigemptyset(&sigaq.sa_mask);

  /* SA_RESTART will continue doing syscalls after ctrlq_handler is finished */
  sigaq.sa_flags = SA_RESTART;

  if (sigaction(SIGQUIT, &sigaq, NULL)) {
    perror("sigaction");
    exit(1);
  }

  while (1) {

    // print prompt
    fprintf(stdout, "%d $ ", getpid());

    // read command from stdin
    // stops when newline is entered or 19 (n - 1) bytes are read
    fgets(buf, 20, stdin);
    len = strlen(buf);
    if (len == 1) // case: only enter/return key pressed
      continue;
    buf[len - 1] = '\0'; // case: command entered
    char *ptr_buf = buf;
    while (*ptr_buf == ' ') {
      ptr_buf++;
    }
    if (*ptr_buf == 'q') {
      exit(0);
    }
    k = fork();
    if (k == 0) {
      // child code
      if (execlp(buf, buf, NULL) == -1) // if execution failed, terminate child
        exit(1);
    } else {
      // parent code
      waitpid(k, &status, 0); // block until child process terminates
    }
  }
}
