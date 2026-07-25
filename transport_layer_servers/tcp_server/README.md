# tcp_server — Password-Authenticated TCP Shell Server

A client-server shell over TCP. The server (`mysh4`) listens on a TCP port, authenticates each connection with a 4-character password, and executes the requested shell command, piping stdout/stderr back to the client over the socket. A new `fork()` is spawned per connection, so the server stays alive across requests.

## Files

| File | Purpose |
|------|---------|
| `mysh_v4.c` | Server — binds a TCP socket, accepts connections, validates the password, forks a child that runs the command with stdout/stderr redirected to the socket |
| `mycl.c` | Client — opens a fresh TCP connection per command, prepends the password, sends the request, reads and prints the response |
| `ctrlc_handler.c` | SIGINT handler for the client — prints `\nUse 'q' to quit.` and re-displays the prompt |
| `ctrlq_handler.c` | SIGQUIT handler for the client — same behavior as the SIGINT handler |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and `exit(-1)` if `condition` is true |
| `invalid_password.c` | `invalid_password(pass)` — returns non-zero if the password is not exactly 4 lowercase-alphanumeric characters |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./mysh4 (server) and ./mycl (client)
```

Open two terminals in the same directory:

**Terminal 1 — start the server first:**
```bash
./mysh4 <password> <port>
```

**Terminal 2 — start the client:**
```bash
./mycl <password> <ip> <port>
```

- `<password>` — exactly 4 characters, lowercase letters and digits only (e.g., `ab12`)
- `<port>` — TCP port number the server listens on (e.g., `5000`)
- `<ip>` — IPv4 address of the server (e.g., `127.0.0.1` for localhost)

The client shows `% ` as its prompt. Enter a command with arguments (e.g., `ls -la`). Type `q` to shut down the client; sending `q` also causes the server to exit. Ctrl-C and Ctrl-\\ are intercepted on the client side and will not kill it — they print `Use 'q' to quit.` instead.

If the client sends the wrong password, the server logs the attempt and closes the connection without executing the command.
