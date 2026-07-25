# music_playing_server — Adaptive UDP Audio Streaming

A client-server audio streaming tool. The server (`musas`) reads a `.au` file and streams it over UDP in 4096-byte chunks, sleeping `ilambda` seconds between sends. The client (`musac`) buffers incoming chunks in a circular buffer and plays them back through the ALSA audio device (mu-law, 8000 Hz, mono). After each playback chunk, the client applies **Control Law D** to compute a new `ilambda` and sends it back to the server, so the send rate adapts to keep the buffer near its target fill level. Up to 2 concurrent streaming sessions are supported.

## Files

| File | Purpose |
|------|---------|
| `musas.c` | Server — listens for TCP connections; on each accepted session, forks a child that opens a UDP socket, streams the audio file at rate `ilambda`, and updates `ilambda` via SIGPOLL whenever the client sends a new value; writes a per-session log of ilambda over time |
| `musac.c` | Client — connects via TCP, negotiates a UDP port, then uses `select()` to receive audio into a circular buffer and play it back every `igamma` µs; applies Control Law D after each playback chunk and sends the updated `ilambda` back to the server; writes a buffer-occupancy log |
| `invalid_filename.c` | `invalid_filename(name)` — returns non-zero if the filename is not 1–12 lowercase alphanumeric characters followed by `.au` |
| `error.c` | `error(condition, msg)` — calls `perror(msg)` and exits with code 1 if `condition` is true |
| `error_out.c` | `error_out(condition, msg)` — prints `msg` and exits with code 1 if `condition` is true (no `perror`) |
| `control-param.dat` | Control Law D parameters read by the client at startup |
| `Makefile` | Build rules (links `musac` against `-lasound` for ALSA) |

Sample audio files included: `db.au`, `kj.au`, `pp.au`

## How to Build

Run `make` from the `music_playing_server/` directory:

```bash
make        # produces ./musas (server) and ./musac (client)
```

Requires the ALSA development library (`libasound`). On Debian/Ubuntu: `sudo apt install libasound2-dev`.

## How to Run

The server must be started before the client.

**Terminal 1 — from `music_playing_server/`, start the server:**
```bash
./musas <server_ip> <port> <ilambda> <logfile>
```

**Terminal 2 — from `music_playing_server/`, start the client:**
```bash
./musac <audio_file> <server_ip> <port> <igamma> <bufsize> <targetbf> <ctrace>
```

### Argument reference

| Argument | Description |
|----------|-------------|
| `<server_ip>` | IPv4 address the server binds to |
| `<port>` | TCP port the server listens on |
| `<ilambda>` | Initial send interval in seconds (e.g. `0.313`) — time the server sleeps between 4096-byte UDP sends |
| `<logfile>` | Base name for per-session server log files; each session prepends its session number (e.g. `1logfile.dat`) |
| `<audio_file>` | `.au` file to stream — 1–12 lowercase alphanumeric characters followed by `.au` (e.g. `db.au`) |
| `<igamma>` | Client playback interval in microseconds (e.g. `313000`) — how often the client plays a 4096-byte chunk |
| `<bufsize>` | Total client circular buffer size in bytes; must be a multiple of 4096 (e.g. `16384`) |
| `<targetbf>` | Target buffer fill in bytes; must be a multiple of 4096 and ≤ `bufsize` (e.g. `8192`) |
| `<ctrace>` | Output file for the client's buffer-occupancy log (e.g. `ctrace.dat`) |

### Example (using defaults from the original README)

```bash
./musas 127.0.0.1 5000 0.313 logfile.dat                    # Terminal 1
./musac db.au 127.0.0.1 5000 313000 16384 8192 ctrace.dat   # Terminal 2
```

`control-param.dat` must be present in the directory where `musac` is run.

## control-param.dat Format

```
<ilambda>
<epsilon>
<beta>
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ilambda` | `0.313` | Initial send rate override from file (seconds/chunk) |
| `epsilon`  | `0.001` | Buffer-level correction gain |
| `beta`     | `0.00001` | Rate correction gain — pulls `ilambda` toward the natural playback rate |

## Control Law D

After each playback chunk the client updates `ilambda` and sends it to the server:

```
ilambda += epsilon × ((Q − targetbf) / bufsize) + beta × ((igamma / 1e6) − ilambda)
```

- If the buffer is too full (`Q > targetbf`), `ilambda` increases → server sends slower
- If the buffer is too empty (`Q < targetbf`), `ilambda` decreases → server sends faster
- The `beta` term pulls `ilambda` back toward the natural playback rate (`igamma / 1e6` seconds/chunk)

Playback does not begin until the buffer reaches `targetbf` bytes.
