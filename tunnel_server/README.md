# tunnel_server — UDP Tunnel Daemon

A UDP tunneling tool. `tunnelcli` registers a tunnel session with the daemon (`tunneld`) over TCP, telling it the source and destination addresses. `tunneld` then acts as a bidirectional UDP relay — forwarding packets from the source to the destination and bouncing replies back — so two hosts that cannot communicate directly can exchange UDP traffic through the tunnel. Up to 5 concurrent sessions are supported.

Intended to be used alongside `upings`/`upingc` from the `udp_ping_server` project, with `tunneld` sitting between them.

## Files

| File | Purpose |
|------|---------|
| `tunneld.c` | Daemon — listens for TCP connections from `tunnelcli`; on each accepted session, forks a child that binds two UDP sockets and uses `select()` to relay packets bidirectionally between source and destination |
| `tunnelcli.c` | Client — connects to `tunneld` via TCP, sends the session request (code 200), password, destination IP+port, and source IP; receives and prints the UDP port that the source should send packets to |
| `invalid_password.c` | `invalid_password(pass)` — returns non-zero if the password is not exactly 5 letters (uppercase or lowercase, no digits) |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and exits with code 1 if `condition` is true |
| `error_out.c` | `error_out(condition, msg)` — prints `msg` and exits with code 1 if `condition` is true (no `perror`) |
| `Makefile` | Build rules |

## How to Build

Run `make` from the `tunnel_server/` directory:

```bash
make        # produces ./tunneld (daemon) and ./tunnelcli (client)
```

## How to Run

This setup requires four machines (or four processes on localhost with different roles):

**Step 1 — start the ping server (`upings` from `udp_ping_server`):**
```bash
./upings <upings_secret> <upings_port>
```

**Step 2 — start the tunnel daemon on its own machine/port:**
```bash
./tunneld <tunneld_ip> <tunneld_tcp_port> <tunneld_secret>
```

**Step 3 — register the tunnel session with `tunnelcli`:**
```bash
./tunnelcli <tunneld_ip> <tunneld_tcp_port> <tunneld_secret> <upingc_ip> <upings_ip> <upings_port>
```
`tunnelcli` prints `Port Number: <N>` — this is the UDP port on `tunneld` that `upingc` must target.

**Step 4 — start the ping client (`upingc` from `udp_ping_server`), pointing at tunneld:**
```bash
./upingc <upings_secret> <tunneld_ip> <port_from_step3> <micsec>
```

### Argument reference

| Argument | Description |
|----------|-------------|
| `<tunneld_ip>` | IPv4 address of the machine running `tunneld` |
| `<tunneld_tcp_port>` | TCP port `tunneld` listens on for `tunnelcli` connections |
| `<tunneld_secret>` | Exactly 5 letters (upper or lowercase) — authenticates `tunnelcli` to `tunneld` |
| `<upingc_ip>` | IPv4 address of the machine running `upingc` (source) |
| `<upings_ip>` | IPv4 address of the machine running `upings` (destination) |
| `<upings_port>` | UDP port `upings` is listening on |
| `<upings_secret>` | 4-character lowercase alphanumeric password used by `upingc`/`upings` |
| `<micsec>` | Inter-ping interval in microseconds (passed to `upingc`) |

## How the Tunnel Works

```
upingc  --UDP-->  tunneld (receive socket, port from tunnelcli)
                  tunneld  --UDP-->  upings
                  tunneld  <--UDP--  upings
upingc  <--UDP--  tunneld
```

`tunneld` maintains a `nexthoptab` table (up to 5 entries). For each session it forks a child that opens two UDP sockets — one facing the source (bound starting at port 56600) and one facing the destination (bound starting at port 57500) — and uses `select()` to relay packets in both directions. Packets from an IP address that does not match the registered source are discarded.
