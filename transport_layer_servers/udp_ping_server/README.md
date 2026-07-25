# udp_ping_server — UDP Ping with RTT Statistics

A UDP ping tool. The client (`upingc`) sends 50 6-byte packets to the server at a configurable interval, recording send and receive timestamps for each. Responses are delivered asynchronously via a SIGPOLL handler. After all pings complete the client prints min, average, and max round-trip times in microseconds.

## Files

| File | Purpose |
|------|---------|
| `upings.c` | Server — binds a UDP socket, validates the 4-byte password prefix on each incoming packet, and echoes the packet back to the sender |
| `upingc.c` | Client — sends 50 ping packets, sleeps between each, waits 750 ms after the last one, then computes and prints RTT statistics |
| `pingrespcb.c` | SIGPOLL handler — called when a packet arrives on the client socket; reads the packet, extracts the sequence number, and records the arrival timestamp in `pingend[]` |
| `inter_sleep.c` | `inter_sleep(ts)` — wraps `nanosleep` in a loop so that a sleep interrupted by SIGPOLL resumes for the remaining duration |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and `exit(-1)` if `condition` is true |
| `invalid_password.c` | `invalid_password(pass)` — returns non-zero if the password is not exactly 4 lowercase-alphanumeric characters |
| `Makefile` | Build rules |

## How to Build and Run

```bash
make        # produces ./upings (server) and ./upingc (client)
```

**Terminal 1 — start the server:**
```bash
./upings <password> <port>
```

**Terminal 2 — start the client:**
```bash
./upingc <password> <ip> <port> <microseconds>
```

- `<password>` — exactly 4 characters, lowercase letters and digits only (e.g., `ab12`)
- `<port>` — UDP port number the server listens on (e.g., `5000`)
- `<ip>` — IPv4 address of the server (e.g., `127.0.0.1` for localhost)
- `<microseconds>` — interval between pings in microseconds; must be ≥ 1 (e.g., `100000` for 100 ms)

After all 50 pings finish and the 750 ms drain window closes, the client prints:

```
Min: <N> microseconds
Avg: <N> microseconds
Max: <N> microseconds
```

Only pings that received a response within 1 second (1 000 000 µs) are counted in the statistics. The server runs indefinitely; kill it manually (Ctrl-C) after the client exits.
