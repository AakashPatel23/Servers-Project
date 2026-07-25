# v2 — No-Fork Shell (Single-Use)

A shell that skips `fork()` and calls `execvp` directly in the main process. Because `execvp` replaces the running process image on success, the shell can only execute one command before it disappears. If `execvp` fails, `exit(1)` is called immediately. This version demonstrates what happens when there is no child process — the shell itself becomes the command.

## Files

| File | Purpose |
|------|---------|
| `mysh_v2.c` | Shell implementation |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./mysh2
./mysh2
```

The shell prints `<PID> $ ` as its prompt. Enter a command with arguments. The shell will execute it and, on success, will not return — the process has been replaced by that command. On failure, the shell exits with code 1.
