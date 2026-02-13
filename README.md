# FTP Proxy Server

This project presents the implementation of a **transparent FTP proxy server** developed in C++ using the Qt framework (QTcpServer and QTcpSocket).

The proxy server acts as an intermediary between an FTP client and a real FTP server. It forwards FTP control commands and server responses without modifying them.

The implementation demonstrates understanding of:

- TCP socket communication
- Client–server architecture
- Event-driven programming using Qt
- FTP protocol fundamentals
- Proxy server design principles

---

## Architecture

The communication flow is structured as follows:

FTP Client → FTP Proxy (port 2121) → FTP Server (port 21)

The proxy:
- Listens for client connections on port 2121
- Establishes a connection to the real FTP server on port 21
- Forwards commands from client to server
- Forwards responses from server to client
- Logs FTP communication
- Blocks TLS/SSL authentication commands

---

## Technologies Used

- C++
- Qt 6 (QTcpServer, QTcpSocket)
- Linux
- Git

---

## Features

- Transparent forwarding of FTP control channel
- Logging of client and server communication
- Graceful connection cleanup
- Basic filtering of unsupported TLS commands

---

## How to Run

1. Make sure an FTP server (e.g., vsftpd) is running on port 21.
2. Build the project using Qt Creator.
3. Run the proxy application.
4. Connect using an FTP client (e.g., FileZilla) to:

   Host: 127.0.0.1  
   Port: 2121  

---

## Educational Purpose

This project was developed as part of the course requirements to demonstrate practical understanding of network communication and protocol-level proxy implementation.
