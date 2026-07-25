# v4 — Client-Server Shell via Named Pipes (FIFOs)

Splits the shell into two separate processes that communicate over a pair of named pipes. The user interacts only with the client (`mycl`); the server (`mysh4`) runs in a separate terminal and never directly touches the user's keyboard or screen. The child in the server redirects its stdout into the response pipe so the client can display it.

## Files

| File | Purpose |
|------|---------|
| `mysh_v4.c` | Server — reads commands from `myfifo4a`, executes them, writes output to `myfifo4b` |
| `mycl.c` | Client — reads user input, writes to `myfifo4a`, reads output from `myfifo4b`, prints it |
| `ctrlc_handler.c` | SIGINT handler for the client — prints `\nUse 'q' to quit.` and re-displays the prompt |
| `ctrlq_handler.c` | SIGQUIT handler for the client — same behavior as the SIGINT handler |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./mysh4 (server) and ./mycl (client)
```

Open two terminals in the same directory:

**Terminal 1 — start the server first:**
```bash
./mysh4
```

**Terminal 2 — start the client:**
```bash
./mycl
```

The client shows `% ` as its prompt. Enter a command with arguments (e.g., `ls -la`). Type `q` to shut down both the client and the server. Ctrl-C and Ctrl-\\ are intercepted on the client side and will not kill it.

The server must be started before the client because the client opens `myfifo4a` for writing, which blocks until a reader (the server) opens the other end.
