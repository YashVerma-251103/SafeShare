# SafeShare LAN: Session-Based Secure File Sharing

**SafeShare** is a consent-first, peer-to-peer file sharing and messaging application designed for Local Area Networks (LAN). Unlike standard FTP or SMB services, SafeShare requires explicit user approval (via a token) before any peer can access files, ensuring security and control. We were inspired by the torrent network model but tailored for trusted environments like home or office networks.

-----

## Networking Concepts Implemented

We built this project to demonstrate mastery of the following networking fundamentals:

### 1\. UDP Broadcasting (Service Discovery)

  * **Concept:** User Datagram Protocol (UDP) is used for connectionless communication.
  * **Implementation:** Nodes broadcast "Announce" packets to `255.255.255.255` on port `55000`.
  * **Why:** Allows devices to automatically find each other on the LAN without a central server or manual IP entry.
  * *Code Reference:* `src/discovery/udp_announcer.cpp` & `udp_listener.cpp`

### 2\. TCP Socket Programming (Reliable Data Transfer)

  * **Concept:** Transmission Control Protocol (TCP) provides reliable, ordered, and error-checked delivery of a stream of octets.
  * **Implementation:** File transfers and permission handshakes occur over TCP sockets (Port `55001`). We utilize blocking I/O with multi-threading to handle multiple concurrent client connections.
  * **Why:** Ensures file integrity; not a single byte is lost or corrupted during transfer.
  * *Code Reference:* `src/server/server.cpp` & `src/client/client.cpp`

### 3\. Custom Application-Layer Protocol & Framing

  * **Concept:** Defining how data is structured so the receiver knows when a message starts and ends.
  * **Implementation:** We implemented a **Length-Prefix Framing** protocol.
      * **Structure:** `[4-byte Length (Big Endian)] + [JSON Header] + [Binary Payload (optional)]`
      * **Serialization:** JSON is used for control messages (`PERM_REQUEST`, `LIST_FILES`), making the protocol extensible and debuggable.
  * *Code Reference:* `src/common/framing.hpp`

### 4\. Session Management & Authentication

  * **Concept:** Stateless protocols need a way to maintain state.
  * **Implementation:** The server issues a **Time-To-Live (TTL) Session Token** upon pairing. This token is cryptographically bound to the client's IP address.
  * **Security:** Prevents unauthorized access. Even if a token is stolen, it cannot be used from a different machine (IP spoofing protection).
  * *Code Reference:* `src/server/token_manager.cpp`

### 5\. Embedded HTTP Server

  * **Concept:** Interaction between a C++ backend and a Web Browser.
  * **Implementation:** A raw socket listener handles HTTP `GET` requests to serve a dashboard on Port `8080`.
  * **Why:** Provides a modern UI for users to "Accept" or "Deny" incoming connection requests in real-time.
  * *Code Reference:* `src/ui/mini_web.cpp`

-----

## System Architecture

The application is designed as a **Hybrid Node**, meaning every instance acts as both a Client and a Server simultaneously.

1.  **Discovery Layer:** Runs on a background thread, listening for peers.
2.  **Control Plane:** Handles permission requests (`PERM_REQUEST`) and file listing (`LIST`).
3.  **Data Plane:** Handles the actual transmission of file chunks (`MSG_FILE_CHUNK`).
4.  **Web Interface:** A dashboard running on `localhost:8080` acts as the "Remote Control" for the C++ backend.

-----

## Build & Run Instructions

### Prerequisites

  * C++17 Compiler (GCC/Clang)
  * CMake (3.16+)
  * OpenSSL & SQLite3 (libraries)

### Compilation

```bash
mkdir build && cd build
cmake ..
make
```

### Usage

**1. Start the Node (Server + Web UI)**

```bash
./src/server/safeshare-node
```

  * Open your browser to: `http://localhost:8080`
  * You will see the dashboard and discovered peers.

### Future Plan 

**1. Add secure Upload File Feature.**  
**2. Add a pause download feature for larger files.**  
**1. Try to replicate the actual torrent working with the file being shared among peers.**  

-----
