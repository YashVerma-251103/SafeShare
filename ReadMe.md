# SafeShare LAN — Design & Architecture

> Consent-first LAN file sharing + messaging system (C / C++)

---

## 1. Project summary

SafeShare LAN is a consent-first local network application that enables users on the same LAN to discover each other, request access to share selected folders, exchange files, and chat — only after the target device explicitly grants permission. The system demonstrates UDP service discovery, TCP/TLS connections, token-based authorization, multithreaded servers, file streaming, integrity checks, and local persistence.

Language: **C or C++** (implementation plan covers both; recommended: **C++17** with Boost.Asio + OpenSSL + SQLite3 for faster, safer development.)

Target LOC: 800–1500 (project easily satisfies 500+ requirement).

---

## 2. Goals & non-goals

### Goals
- Demonstrate core network programming concepts (UDP discovery, TCP, TLS, framing, concurrency).  
- Provide a clear consent flow for sharing.  
- Implement file listing, chunked transfer, resumable downloads, and chat.  
- Keep filesystem access strictly limited to configured shared directories.  
- Provide simple CLI + small embedded web UI for admin and consent.

### Non-goals
- No remote shell or arbitrary filesystem browsing beyond shared folders.  
- Not a production hardened system; reasonable security assumptions for LAN demo.

---

## 3. Threat model and ethics

- Threats considered: unauthorized file access, token theft, man-in-the-middle, directory traversal.  
- Countermeasures: TLS, token scoping + TTL + revocation, path normalization, explicit user consent, local logs, sandboxing of shared folders, rate limits, and size limits.  

Ethics: Every share requires explicit owner consent via an interactive prompt. Document includes clear privacy & consent UI guidance.

---

## 4. High-level architecture (components)

```
+--------------------+          UDP Broadcast/Listen         +--------------------+
| Device A (server)  | <-----------------------------------> | Device B (client)  |
|  - Discovery       |                                      |  - Discovery       |
|  - ShareServer     | TCP/TLS                            TCP/TLS  - Client      |
|  - TokenManager    | <-- PERM_REQUEST / PERM_RESPONSE -->|  - TokenStore      |
|  - FileManager     | <-- LIST / DOWNLOAD / UPLOAD  -->   |  - ChatClient      |
|  - ChatServer      | <-- CHAT messages -------------->   |  - UI              |
+--------------------+                                      +--------------------+
            |                                                            |
            | Local DB (SQLite)                                          |
            +------------------------------------------------------------+
```

Components:
- Discovery (UDP announce + listener)
- TCP TLS service(s): FileServer + ChatServer (can be combined into single multi-service server)
- TokenManager (issue, validate, revoke short-lived tokens)
- Auth & Consent UI (local popup / embedded HTTP consent page)
- Client module (discovery, request, chat, file transfer client)
- Local persistence (SQLite): tokens, transfers, message history, config
- Optional: small embedded web UI (HTTP server) for easier demo

---

## 5. Networking & protocol

### Service discovery
- Protocol: UDP broadcast on known port (e.g., 55000) or multicast.  
- Announcement payload minimal: `{ device_id, name, service_port, tls_pubkey_fingerprint }` (JSON or compact TLV).  
- Listener marks device as `NeedsConsent` until a permission token is issued.

### Application transport
- Transport: TCP over TLS (OpenSSL). One or two ports:
  - Port A: control & messaging
  - Port B (optional): high-throughput file transfer
- Framing: 4-byte big-endian length prefix, followed by a JSON header (UTF-8), optional binary payloads. Example frame:
  - `[u32 len][json-header][binary-chunk?]`

### Core message types (JSON header)
- `PERM_REQUEST` — client -> peer: `{type:"PERM_REQUEST", from_device:"id", display_name:"user", reason:"string"}`
- `PERM_RESPONSE` — peer -> client: `{type:"PERM_RESPONSE", status:"ACCEPT|DENY", token:"...", scope:["/shared/docs"], ttl:seconds}`
- `LIST` — client -> peer: `{type:"LIST", token:"...", path:"/shared/docs"}`
- `LIST_RESP` — peer -> client: `{type:"LIST_RESP", entries:[{name,size,is_dir,mtime,checksum?}], status:OK}`
- `DOWNLOAD_REQ` — `{type:"DOWNLOAD_REQ", token:"...", path:"x.pdf", offset:0}`
- `DOWNLOAD_CHUNK` — contains binary payload chunks sent after JSON header or as separate frames
- `CHAT_SEND` / `CHAT_ACK` — chat messages framed similarly
- `ERROR` — standardized error reply with `code` and `message`

All messages must include a `nonce`/`id` (UUID) for idempotency and tracing.

---

## 6. Token & authorization model

- Token type: opaque random token with server-side store (safer to implement). Token metadata: `token_id, issued_to_device, scope, permissions, expires_at, issued_by`.
- Token lifecycle: created on `PERM_REQUEST` acceptance, TTL default 15 minutes, optional "Always allow" which creates long-lived pairing token (persisted and revocable).  
- Validation: Token store is checked for scope & permitted operation on every request.
- Revocation: server keeps a blacklist map and invalidates tokens immediately.

---

## 7. Security details

- TLS: OpenSSL library. Generate self-signed cert per device at first run; present TLS cert fingerprint in discovery to aid owner verification.
- Optional mutual TLS: for "Always allow" paired devices generate and exchange client certs for mutual TLS.
- Path normalization and sandboxing: resolve requested path under allowed shared directories, reject traversal attempts.
- Checksums: SHA-256 for file chunks; verify after complete transfer.
- Rate limiting: per-IP and per-token concurrent transfer limit, and per-second rate caps.

---

## 8. Concurrency & architecture choices (C vs C++)

### C++ (recommended)
- Networking library: **Boost.Asio** (async I/O) or plain POSIX sockets + epoll/kqueue for performance.  
- Threading: std::thread + thread pool for CPU-bound tasks (checksum), asio uses io_context threads for I/O.  
- TLS: OpenSSL wrapped manually or via asio-openssl examples.  
- Persistence: SQLite (sqlite3 C API) with a small wrapper.

### C (alternative)
- Sockets: POSIX sockets + `select()` or `epoll()` on Linux.  
- Threading: pthreads for concurrent handlers.  
- TLS: OpenSSL.  
- Persistence: sqlite3 C API.

Concurrency model (recommended):
- Main reactor (epoll / asio io_context) for accepting connections and reading frames.  
- A thread pool for heavy tasks (checksum, file I/O, DB writes).  
- Per-connection state object storing partial frames, token, peer id.

---

## 9. File transfer details

- Chunk size: 64 KiB default (configurable).  
- Resume support: client issues `DOWNLOAD_REQ` with `offset`; server serves from offset. Server maintains partial-transfer metadata (temp file + map).  
- Integrity: per-chunk SHA-256 optional + final file SHA-256.  
- Atomic write: write to `.tmp` file then `rename()` on completion.

---

## 10. Chat / messaging layer

- Reuse same TLS connection after token issuance. Chat frames are small JSON messages with `msg_id`, `from`, `to`, `seq`, `body`.  
- Delivery and read receipts: `CHAT_ACK` with status.  
- Offline: sender stores pending messages locally; optionally the peer may run a message queue if owner opts-in to accept offline messages (explicit opt-in).  
- Group chat: group id with membership list; invites follow consent flow.

---

## 11. Local DB schema (SQLite) — minimal

Tables:
- `devices(device_id TEXT PRIMARY KEY, name TEXT, last_seen INTEGER, fingerprint TEXT)`
- `tokens(token TEXT PRIMARY KEY, device_id TEXT, scope TEXT, expires_at INTEGER, persistent BOOL)`
- `shares(share_id INTEGER PRIMARY KEY, path TEXT, alias TEXT, owner BOOL)`
- `messages(id TEXT PRIMARY KEY, conv_id TEXT, from_dev TEXT, to_dev TEXT, body TEXT, status TEXT, ts INTEGER)`
- `transfers(id TEXT PRIMARY KEY, filename TEXT, path TEXT, size INTEGER, offset INTEGER, status TEXT, ts INTEGER)`

---

## 12. Error handling & codes

Standardize a small set of codes for `ERROR` messages: `401 UNAUTHORIZED`, `403 FORBIDDEN`, `404 NOT_FOUND`, `413 PAYLOAD_TOO_LARGE`, `429 RATE_LIMIT`, `500 INTERNAL`. Include `error_id` for tracing.

---

## 13. Testing & QA

- Unit tests: framing, token validation, path normalization.  
- Integration tests (manual or scripted): two VMs/containers on same network demonstrate discovery, permission flow, file download, resume, and chat.  
- Negative tests: invalid token, revoked token, directory traversal attempt, abrupt connection termination, large file transfer.  
- Performance test: transfer multiple files concurrently to measure throughput.

---

## 14. Milestones & timeline (development slices)

1. **M0 - Setup & tooling**: repo, build system (Makefile/CMake), dependencies (OpenSSL, sqlite3), simple README.
2. **M1 - Discovery & simple control channel**: UDP announce + discover, simple TCP accept + auth handshake, CLI to list devices.
3. **M2 - Permission flow & token manager**: PERM_REQUEST/PERM_RESPONSE, consent UI (terminal or web UI), token store.
4. **M3 - File listing & single-file download**: LIST + DOWNLOAD, chunked transfer, simple progress reporting.
5. **M4 - Resume & checksum**: implement resume and checksums.
6. **M5 - Chat layer**: CHAT_SEND/ACK, message persistence.
7. **M6 - Security & polish**: TLS, path checks, rate limits, logging, test suite.
8. **M7 - Documentation + webpage + demo video**

Each milestone maps to deliverables for grading.

---

## 15. Deliverables mapping (what to submit)
- Project document: this design + implementation notes + protocol spec.  
- Source code: well-commented C/C++ code, Makefile/CMake, README with run steps.  
- Webpage: static HTML summarizing feature list + architecture diagram + run demo GIFs/screenshots.  
- Demo video: 2-min recording following the demo script in section 16.  
- Public link: GitHub repo with README, wiki, and release with demo.

---

## 16. Demo script (2 minutes)
1. Start Device A, show config and enabled shared folder.  
2. Start Device B; B discovers A (greyed).  
3. B clicks request → A shows consent prompt → A accepts for 15 minutes.  
4. B lists files from A and starts a download → show progress on both sides and final checksum match.  
5. Open chat from B to A; send a short message and show ACK.  
6. Revoke token from A and show subsequent request rejected.

---

## 17. Folder structure (suggested)

```
/safeshare
  /src
    /common     # framing, utils, crypto
    /discovery  # UDP announce/listener
    /server     # file + chat server
    /client     # CLI client
    /ui         # embedded web UI (static html + handler)
  /tests
  /docs
  CMakeLists.txt
  README.md
```

---

## 18. Build & dependencies

Recommended: C++17 + libraries:
- Boost (optional) — Boost.Asio
- OpenSSL
- sqlite3
- nlohmann/json (header-only) or cJSON (C) for JSON
- CMake for build

If using pure C: POSIX sockets + OpenSSL + sqlite3 + cJSON.

---

## 19. Implementation notes & coding conventions

- Keep network framing deterministic; always read length-prefixed frames.  
- Use RAII (C++) to manage resources.  
- Keep code modular: protocol, transport, application logic separated.  
- Comment public functions and protocol behaviour.  
- Add debug logging with levels (INFO/DEBUG/WARN/ERROR) toggled at runtime.

---

## 20. Example: PERM_REQUEST sequence (text diagram)

1. B sends UDP discovery -> A sees announcement.  
2. B opens TLS connection to A: sends `PERM_REQUEST`.  
3. A shows owner popup (or web UI).  
4. Owner clicks ACCEPT with scope `/shared/docs`.  
5. A generates token `t1`, stores it, returns `PERM_RESPONSE {status:ACCEPT, token:t1, ttl:900}`.  
6. B uses `t1` to call `LIST` and `DOWNLOAD`.

---

## 21. Logging & privacy

- Local logs should contain minimal personal info and be stored under user directory with rotation.  
- Offer a "Clear logs" button in UI.  

---

## 22. Grading hook & bonus suggestions

- Implement resumable transfers + integrity = good marks.  
- Add mutual TLS pairing / device certificate exchange = bonus.  
- Add group chat + offline queue storage (scoped opt-in) = more credit.

---

## 23. Next steps (developer actions)

- Choose language: **C++ (recommended)** or C.  
- I will generate: minimal skeleton code for discovery + permission + token manager + LIST endpoint in your chosen language.  
- Prepare repo skeleton and Makefile/CMake.

---

## Appendix: minimal JSON message examples

- `PERM_REQUEST`:
```json
{"type":"PERM_REQUEST","from":"dev-b","display_name":"Ravi","reason":"share notes"}
```

- `PERM_RESPONSE` (accept):
```json
{"type":"PERM_RESPONSE","status":"ACCEPT","token":"3f2a...","scope":["/shared/docs"],"ttl":900}
```

- `LIST`:
```json
{"type":"LIST","token":"3f2a...","path":"/shared/docs"}
```

- `CHAT_SEND`:
```json
{"type":"CHAT_SEND","id":"uuid","token":"...","to":"dev-a","body":"hi"}
```

---
