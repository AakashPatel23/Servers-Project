# file_transfer_server — UDP File Transfer with Redundancy and Simulated Drops

A UDP-based file transfer tool. The sender (`tripfs`) breaks a file into fixed-size blocks and transmits each block 3 times for redundancy, with optional artificial packet drops configured via `param.dat`. The receiver (`tripfr`) reassembles the blocks in order, writes the result to `received_file`, reports any missing blocks, and prints transfer time. Both sides use SIGALRM timeouts to detect an unreachable peer.

## Files

| File | Purpose |
|------|---------|
| `send/tripfs.c` | Sender — reads `param.dat` for drop rules, sends 3 control packets with file size and block size, waits for an ACK, then sends each file block 3 times (skipping sends where `dropornot` returns 1); prints transmission time |
| `receive/tripfr.c` | Receiver — receives 3 control packets to learn file size and block size, sends back an ACK, reassembles incoming blocks into `received_file`, reports any blocks that arrived fewer than 3 times, and prints receiving time |
| `send/alarmcb.c` | SIGALRM handler for the sender — exits with `"Receiver is not reachable!"` if the ACK timeout (3 s) fires |
| `receive/alarmcb.c` | SIGALRM handler for the receiver — exits with `"Sender is not reachable!"` if the data timeout (7 s) fires |
| `send/dropornot.c` | `dropornot(seq)` — returns 1 if the block at sequence number `seq` should be dropped this send, per the rules loaded from `param.dat`; decrements the rule's counter each time it fires |
| `send/invalid_filename.c` | `invalid_filename(name)` — returns non-zero if the filename is not exactly 8 characters of lowercase letters, digits, or dots |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and exits with code 1 if `condition` is true |
| `error_out.c` | `error_out(condition, msg)` — prints `msg` and exits with code 1 if `condition` is true (no `perror`) |
| `send/param.dat` | Drop configuration read by the sender at startup |
| `Makefile` | Build rules (run from the `file_transfer_server/` directory) |

## How to Build and Run

Run `make` from the `file_transfer_server/` directory:

```bash
make        # produces ./tripfs (sender) and ./tripfr (receiver)
```

The receiver must be started before the sender — it must be listening before the sender transmits control packets.

**Terminal 1 — from `file_transfer_server/`, start the receiver:**
```bash
./tripfr <port>
```

**Terminal 2 — from `send/`, start the sender (it reads `param.dat` from the current directory):**
```bash
cd send
../tripfs <filename> <ip> <port> <microseconds> <block_size>
```

- `<filename>` — exactly 8 characters, lowercase letters, digits, or dots (e.g., `file.txt`)
- `<ip>` — IPv4 address of the receiver (e.g., `127.0.0.1` for localhost)
- `<port>` — UDP port the receiver is listening on
- `<microseconds>` — sleep between packet sends in microseconds; use `0` for no delay
- `<block_size>` — bytes per block; must be ≥ 1

### Example

```bash
# Terminal 1 — from file_transfer_server/
./tripfr 5000

# Terminal 2 — from file_transfer_server/send/
cd send && ../tripfs file.txt 127.0.0.1 5000 0 100
```

## Output

**Sender** prints:
```
Transmission Time: <N> ms
```

**Receiver** prints any blocks that arrived fewer than 3 times, then:
```
Receiving Time: <N> ms
```

The reassembled file is written to `received_file` in the directory where `tripfr` is run.

## param.dat Format

```
<num_rules>
<seq_num> <drop_count>
...
```

Each rule tells the sender to skip `drop_count` transmissions of the block at `seq_num`. Since every block is normally sent 3 times, a `drop_count` of 2 means only 1 copy gets through; a `drop_count` of 3 drops the block entirely. Up to 5 rules are supported.

**Example (`send/param.dat`):**
```
3
1 2
3 2
4 1
```
Block 1 loses 2 of its 3 sends, block 3 loses 2, block 4 loses 1.
