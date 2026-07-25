// Concurrent server example: simple shell using fork() and execlp().

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NAME_A "./myfifo4a"
#define NAME_B "./myfifo4b"
#define LEN (1024)

void ctrlc_handler(int);
void ctrlq_handler(int);

int main(void) {
  char buf[20];
  char output[LEN];
  int len;
  int fda;
  int fdb;

  /* Makes FIFO B named pipe */

  mkfifo(NAME_B, 0666);

  /* Sets up SIGINT signal handler */

  struct sigaction siga;
  siga.sa_handler = ctrlc_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&siga.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  siga.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &siga, NULL)) {
    perror("sigaction");
    exit(1);
  }

  /* Sets up SIGQUIT signal handler */

  struct sigaction sigaq;
  sigaq.sa_handler = ctrlq_handler;

  /* Sets sa_mask to empty so that no other signals are blocked while handler
   * runs */

  sigemptyset(&sigaq.sa_mask);

  /* SA_RESTART will continue doing syscalls after handler is finished */

  sigaq.sa_flags = SA_RESTART;

  if (sigaction(SIGQUIT, &sigaq, NULL)) {
    perror("sigaction");
    exit(1);
  }

  /* Opens FIFO A named pipe to write to */

  fda = open(NAME_A, O_WRONLY);

  while (1) {

    // print prompt
    fprintf(stdout, "%% ");

    // read command from stdin
    // stops when newline is entered or 19 (n - 1) bytes are read
    fgets(buf, 20, stdin);
    len = strlen(buf);
    if (len == 1) // case: only enter/return key pressed
      continue;
    buf[len - 1] = '\0'; // case: command entered
    char *ptr_buf = buf;

    /* Gets rid of leading spaces */

    while (*ptr_buf == ' ') {
      ptr_buf++;
    }

    /* Closes server and client */

    if (*ptr_buf == 'q') {
      write(fda, "q\0", 2);
      close(fda);
      exit(0);
    }

    /* Writes user command input to server using pipe A */

    write(fda, ptr_buf, strlen(ptr_buf));
    write(fda, "\n", 1);

    /* Reads server command output from pipe B */

    int num_bytes = 0;
    char c = '\0';
    fdb = open(NAME_B, O_RDONLY);
    while (read(fdb, &c, 1) > 0 && num_bytes < LEN - 1) {
      output[num_bytes] = c;
      num_bytes++;
    }
    output[num_bytes] = '\0';
    close(fdb);

    /* Print output to terminal */

    fprintf(stdout, "%s", output);
  }

  close(fda);
}
