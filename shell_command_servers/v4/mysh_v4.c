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
#define LEN (32)

int main(void) {
  int k;
  int status;
  int fda;
  int fdb;

  /* Make FIFO A named pipe and open it to read */

  mkfifo(NAME_A, 0666);
  fda = open(NAME_A, O_RDONLY);

  while (1) {

    /* Read client command request */

    char command[LEN];
    char c = '\0';
    int num_bytes = 0;

    while (read(fda, &c, 1) > 0 && c != '\n' && num_bytes < LEN - 1) {
      command[num_bytes] = c;
      num_bytes++;
    }
    command[num_bytes] = '\0';

    /* Exits server given quit command */

    if (num_bytes > 0) {
      if (!strcmp(command, "q")) {
        close(fda);
        exit(0);
      }

      char *args[LEN] = {};
      int num_args = 0;
      char *ptr_buf = command;

      /* skip spaces */
      while (*ptr_buf == ' ') {
        ptr_buf++;
      }

      /* keep track of beginning of a word */
      char *start = ptr_buf;

      while (*ptr_buf) {
        /* find and terminate end of a word */
        if (*ptr_buf == ' ') {
          *ptr_buf = '\0';
          args[num_args] = start;
          num_args++;
          ptr_buf++;
          while (*ptr_buf == ' ') {
            ptr_buf++;
          }
          start = ptr_buf;
        } else {
          ptr_buf++;
        }
      }

      /* if word is empty, don't count it as an argument */
      if (*start) {
        args[num_args] = start;
        args[num_args + 1] = NULL;
      } else {
        args[num_args] = NULL;
      }

      k = fork();
      if (k == 0) {
        // child code
        /* Make command write to pipe B */

        fdb = open(NAME_B, O_WRONLY);
        dup2(fdb, 1);
        close(fdb);
        close(fda);
        if (execvp(args[0], args) == -1) // if execution failed, terminate child
          exit(1);
      } else {
        // parent code
        waitpid(k, &status, 0); // block until child process terminates
      }
    }
  }
  close(fdb);
  close(fda);
}

// used by client to send requests to server
// client = writer and server = reader
// myfifo4a = mkfifo(name/path, mode);

// created by client in mycl.c; used by server to respond to client
// client = reader and server = writer
// myfifo4b = mkfifo(name/path, mode);

// mysh

// gets command from mycl (pipe from and read())

// executes command

// returns output to mycl (pipe to and write())
// dup2(myfifo4b, 1); close(myfifo4b);

// mycl

// gets command from user (fread())

// sends to mysh (pipe to and write())

// gets response from mysh (pipe from and read())

// outputs to user (fprintf())
