# v3 — Fork-Based Shell with Signal Handling

Extends v1 with SIGINT (Ctrl-C) and SIGQUIT (Ctrl-\\) signal handlers so those keystrokes print a message instead of terminating the shell. Also adds `q` as an explicit quit command. Uses `execlp` instead of `execvp`, so only single-word commands (no arguments) are supported.

## Files

| File | Purpose |
|------|---------|
| `mysh_v3.c` | Shell main loop and signal setup |
| `ctrlc_handler.c` | SIGINT handler — prints `\nUse 'q' to quit.` and re-displays the prompt |
| `ctrlq_handler.c` | SIGQUIT handler — same behavior as the SIGINT handler |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./mysh_exe
./mysh_exe
```

The shell prints `<PID> $ ` as its prompt. Enter a single-word command (e.g., `ls`). Type `q` to exit cleanly. Ctrl-C and Ctrl-\\ are intercepted and will not kill the shell — they print `Use 'q' to quit.` instead.
