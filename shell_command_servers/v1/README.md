# v1 — Fork-Based Shell with Argument Parsing

A minimal Unix shell that forks a child process for each command, supports multiple arguments, and waits for each child to finish before prompting again.

## Files

| File | Purpose |
|------|---------|
| `mysh_v1.c` | Shell implementation |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./mysh1
./mysh1
```

The shell prints `<PID> $ ` as its prompt. Type any command with arguments (e.g., `ls -la`). Press Enter with no input to re-prompt. Type Ctrl-C or Ctrl-\ to terminate (no signal handling in this version).

