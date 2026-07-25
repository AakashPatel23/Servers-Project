# load_balancing_server — Multi-Path UDP Tunnel with Load Balancing

Extends the tunnel server with **load balancing across multiple paths**. A single `tunneld` session can register up to 5 destinations for the same source; when more than one is registered, incoming packets are forwarded in round-robin fashion — 5 packets per destination — across all registered next hops. The `nexthoptab` is allocated with `mmap(MAP_SHARED)` so the parent process can add new destinations to a running child's session without restarting it.

The intended topology routes `upingc` through two parallel 2-hop paths to `upings`, with `tunneld` at each hop and `tunnelcli` wiring them up:

```
upingc (S) ──► hop1 ──[5 pkts each, round-robin]──► hop3 ──► hop4 ──► upings (D)
                                                   ──► hop2 ──► hop4 ──► upings (D)
```

After 10 seconds of inactivity, each `tunneld` child prints per-destination packet counts and exits.

## Files

| File | Purpose |
|------|---------|
| `tunneld.c` | Tunnel daemon — listens for TCP connections from `tunnelcli`; uses `mmap(MAP_SHARED)` for the session table so the parent can append destinations to a live child's session; the child uses `select()` to relay packets bidirectionally and round-robins across registered destinations 5 packets at a time; prints stats on 10 s timeout |
| `tunnelcli.c` | Tunnel client — connects to a `tunneld`, sends session request (code 200), password, destination IP+port, and source IP; receives and prints the UDP port the source should target |
| `upings.c` | Ping server — echoes 6-byte UDP packets back to sender after verifying the 4-char password prefix |
| `upingc.c` | Ping client — sends 50 pings with configurable spacing, collects RTTs via SIGPOLL, and prints min/avg/max |
| `pingrespcb.c` | SIGPOLL handler for `upingc` — reads the response packet and records the arrival timestamp |
| `inter_sleep.c` | `inter_sleep(ts)` — wraps `nanosleep` in a loop so SIGPOLL interruptions don't cut the sleep short |
| `tripfs.c` | (Bonus) File sender — same as `file_transfer_server` sender; reads `param.dat` for drop rules and transmits file blocks 3× each |
| `tripfr.c` | (Bonus) File receiver — reassembles blocks and writes `received_file`; reports missing blocks and receive time |
| `dropornot.c` | `dropornot(seq)` — returns 1 if a block should be dropped per `param.dat` rules |
| `invalid_filename.c` | `invalid_filename(name)` — returns non-zero if name is not 1–12 lowercase alphanumeric chars + `.au` |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and exits with code 1 |
| `error_out.c` | `error_out(condition, msg)` — prints `msg` and exits with code 1 (no `perror`) |
| `param.dat` | Drop rules for `tripfs`; `0` means no drops |
| `Makefile` | Build rules |

## How to Build

Run `make` from the `load_balancing_server/` directory:

```bash
make    # produces: tunneld, tunnelcli, upings, upingc, tripfs, tripfr
```

## How to Run

Two different passwords are used:

| Password | Format | Used by |
|----------|--------|---------|
| `<tunnel_secret>` | Exactly 5 letters (upper or lowercase) | `tunneld` / `tunnelcli` |
| `<ping_secret>` | Exactly 4 lowercase alphanumeric chars | `upings` / `upingc` |

### Step 1 — Start the destination ping server (machine D)

```bash
./upings <ping_secret> <dest_port>
```

### Step 2 — Start tunneld on all four hop machines (order doesn't matter)

```bash
./tunneld <hop4_ip> <hop4_port> <tunnel_secret>   # machine hop4
./tunneld <hop3_ip> <hop3_port> <tunnel_secret>   # machine hop3
./tunneld <hop2_ip> <hop2_port> <tunnel_secret>   # machine hop2
./tunneld <hop1_ip> <hop1_port> <tunnel_secret>   # machine hop1
```

### Step 3 — Wire up the paths with tunnelcli (machine S, in order)

Each `tunnelcli` call prints a port number — these are used as inputs to later calls. Run them **in this exact order**:

```bash
# [1] Tell hop4: forward packets from hop3 to the destination
./tunnelcli <hop4_ip> <hop4_port> <tunnel_secret> <hop3_ip> <dest_ip> <dest_port>
# → prints P1

# [2] Tell hop4: forward packets from hop2 to the destination
./tunnelcli <hop4_ip> <hop4_port> <tunnel_secret> <hop2_ip> <dest_ip> <dest_port>
# → prints P2

# [3] Tell hop3: forward packets from hop1 to hop4 (via P1)
./tunnelcli <hop3_ip> <hop3_port> <tunnel_secret> <hop1_ip> <hop4_ip> <P1>
# → prints P3

# [4] Tell hop2: forward packets from hop1 to hop4 (via P2)
./tunnelcli <hop2_ip> <hop2_port> <tunnel_secret> <hop1_ip> <hop4_ip> <P2>
# → prints P4

# [5] Tell hop1: forward packets from src to hop3 (via P3)
./tunnelcli <hop1_ip> <hop1_port> <tunnel_secret> <src_ip> <hop3_ip> <P3>
# → prints P5

# [6] Tell hop1: add hop2 as a second destination for the same src (via P4)
./tunnelcli <hop1_ip> <hop1_port> <tunnel_secret> <src_ip> <hop2_ip> <P4>
# → prints the same port as P5 (same source, same session)
```

Steps [5] and [6] target the same source IP, so `tunneld` on hop1 adds hop2 as a second destination to the existing session rather than creating a new one. Both return the same port.

### Step 4 — Start upingc (machine S)

```bash
./upingc <ping_secret> <hop1_ip> <P5> <micsec>
```

hop1 will now load-balance: 5 packets → path via hop3, then 5 packets → path via hop2, repeating until all 50 pings are sent.

## Output

**upingc** prints min/avg/max RTT after all 50 pings complete:
```
Min: <N> microseconds
Avg: <N> microseconds
Max: <N> microseconds
```

**tunneld** on each load-balancing hop prints per-path stats after 10 seconds of inactivity:
```
Source machine (<src_ip>) sent <N> packets to destination machine (<dst_ip>) through <hop_ip>
```

## Bonus: File Transfer over the Tunnel

Replace `upings` with `tripfr` and `upingc` with `tripfs`. Use the same tunneld/tunnelcli setup above.

**Machine D — start the receiver:**
```bash
./tripfr <dest_port>
```

**Machine S — send a file:**
```bash
./tripfs <filename> <hop1_ip> <P5> 10000 1400
```

- `<filename>` — 1–12 lowercase alphanumeric chars + `.au` extension
- `10000` — 10 000 µs inter-packet spacing (recommended)
- `1400` — block size in bytes (from lab spec)

`param.dat` must be present in the working directory. The included `param.dat` contains `0` (no drops). The receiver writes the reassembled file to `received_file`.
