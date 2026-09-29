# FTP Proxy Server

This repository contains the implementation of an asynchronous, multi-client **FTP proxy server** developed in C++ using the Qt framework (`QTcpServer` and `QTcpSocket`).

The proxy operates as an intermediary between FTP clients and an upstream FTP server, forwarding control commands, handling dynamic passive data channels (`PASV` / `EPSV`), and enforcing connection isolation across concurrent sessions.

---

## Key Features

* **Asynchronous Session Management**: Handles multiple concurrent client connections via session-isolated `ProxySession` workers.
* **TCP Stream Framing**: Buffers incoming byte streams to process FTP control commands strictly on `\r\n` line boundaries.
* **Passive Data Tunneling**:
* Intercepts and parses `227 Entering Passive Mode` and `229 Entering Extended Passive Mode` responses.
* Dynamically binds local data proxy listeners to forward data channel traffic during directory listing (`LIST`) and file transfer operations (`RETR`, `STOR`).


* **TLS Fallback Handling**: Explicitly rejects unsupported `AUTH TLS` and `AUTH SSL` requests to prevent command pipeline corruption.
* **CLI Configuration & Logging**: Includes command-line options for host and port configuration alongside structured logging categories.

---

## System Architecture

```
[ FTP Client ]
      │
      │ Control Channel (Port 2121)
      ▼
┌────────────────────────────────────────────────────────┐
│ FTPProxy Listener                                      │
│   └── ProxySession                                     │
│         ├── Control Channel (PASV/EPSV Rewriting)      │
│         └── Dynamic Data Proxy Server                  │
└──────────────────────────┬─────────────────────────────┘
                           │
                           │ Upstream Control Channel (Port 21)
                           ▼
                 [ Upstream FTP Server ]

```

---

## Technical Stack

* **Language**: C++17
* **Framework**: Qt 6 (`Qt::Network`, `Qt::Core`)
* **Build System**: CMake 3.16+

---

## Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release

```

---

## Usage

```bash
./QtFtpProxy [options]

```

### Command-Line Arguments

| Flag | Long Option | Default | Description |
| --- | --- | --- | --- |
| `-p` | `--port` | `2121` | Local port for the proxy to listen on |
| `-t` | `--target` | `127.0.0.1` | IP address or hostname of the target FTP server |
| `-r` | `--remote-port` | `21` | Control port of the target FTP server |
| `-h` | `--help` | — | Display command-line options |
| `-v` | `--version` | — | Display application version |

---

## Testing

Establish a connection using `curl` or any FTP client configured for unencrypted (plain) FTP:

```bash
curl -v ftp://127.0.0.1:2121/ --user username:password

```

---

## License

This project is licensed under the MIT License.
