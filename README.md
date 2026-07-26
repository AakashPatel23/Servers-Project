# CS 422 — Systems Programming Projects

A series of progressively complex systems programming projects written in C, covering Unix process management, network programming, and feedback-driven adaptive systems. Each project builds on the last, culminating in a multi-node load-balanced network with real-time flow control.

**Skills demonstrated:** C, POSIX sockets (TCP/UDP), `fork`/`exec`, `select`, signals, IPC, named pipes, adaptive algorithms, multi-hop networking

---

## Projects

### 1. Shell Command Servers (`shell_command_servers/`)

Four iterative implementations of a Unix shell, each adding a layer of complexity:

| Version | Description |
|---------|-------------|
| v1 | Fork-based shell — `fork` + `execvp` with multi-argument parsing |
| v2 | No-fork shell — calls `execvp` directly, replacing the process image |
| v3 | Signal-handling shell — intercepts SIGINT/SIGQUIT so Ctrl-C/Ctrl-\ print a message instead of killing the shell |
| v4 | Client-server shell over named pipes (FIFOs) — user types commands in a client terminal; a server in a separate terminal executes them and pipes output back |

**Key concepts:** process lifecycle, `execvp`, `sigaction`, inter-process communication via FIFOs

---

### 2. Transport Layer Servers (`transport_layer_servers/`)

Two network servers demonstrating the contrast between TCP and UDP:

- **TCP Shell Server** — a password-authenticated remote shell over TCP. The server forks a child per connection, redirecting stdout/stderr through the socket so command output streams back to the client.
- **UDP Ping Server** — sends 50 timestamped UDP pings with configurable spacing. Responses are collected asynchronously via a SIGPOLL handler and used to compute min/avg/max round-trip time.

**Key concepts:** TCP/UDP socket programming, `fork`-per-connection server model, async I/O with SIGPOLL

---

### 3. File Transfer Server (`file_transfer_server/`)

A UDP-based reliable file transfer tool built without a transport-layer reliability protocol. The sender breaks files into fixed-size blocks and transmits each block three times for redundancy, with artificial packet drops configurable via a parameter file to simulate lossy networks. The receiver reassembles blocks in sequence, reports any gaps, and measures transfer time.

**Key concepts:** UDP reliability without TCP, redundant transmission, packet loss simulation, network timing measurement

---

### 4. Tunnel Server (`tunnel_server/`)

A UDP tunnel daemon that transparently relays traffic between two hosts that cannot communicate directly. A lightweight TCP handshake (using a 5-letter secret key) registers a session with the daemon, which then forks a child to bidirectionally relay UDP packets using `select`. Designed to work with the UDP ping server to measure latency through the tunnel.

**Key concepts:** TCP control plane + UDP data plane, `select`-based multiplexing, transparent packet forwarding, multi-machine network design

---

### 5. Music Playing Server (`music_playing_server/`)

An adaptive UDP audio streaming system. The server streams `.au` audio files (mu-law, 8 kHz, mono) over UDP at a rate controlled by `ilambda` (seconds per chunk). The client buffers incoming audio in a circular buffer, plays it back through the ALSA audio subsystem, and after each playback chunk applies **Control Law D** — a feedback algorithm that adjusts the server's send rate based on buffer occupancy:

```
ilambda += ε × ((Q − Q_target) / Q_max) + β × ((γ / 1e6) − ilambda)
```

This drives the buffer toward its target fill level, preventing both underruns and overflow without a centralized controller.

**Key concepts:** real-time streaming, adaptive rate control, ALSA audio, circular buffers, feedback control systems, SIGPOLL

---

### 6. Load Balancing Server (`load_balancing_server/`)

Extends the tunnel daemon with multi-path load balancing. A single tunnel session can register up to 5 downstream destinations for the same source; incoming packets are distributed across them in round-robin fashion, 5 packets at a time. Session state is shared between parent and child processes via `mmap(MAP_SHARED | MAP_ANON)`, allowing new paths to be added to a live session without restarting it. After 10 seconds of inactivity the daemon prints per-path packet counts.

The full deployment spans six machines: two parallel 2-hop paths (source → hop1 → hop3 → hop4 → destination and source → hop1 → hop2 → hop4 → destination), wired together with six sequential `tunnelcli` invocations. A bonus mode replaces the ping client/server with the file transfer tools to stress-test throughput across the load-balanced paths.

**Key concepts:** load balancing, shared memory with `mmap`, multi-hop routing, round-robin scheduling, multi-machine orchestration
