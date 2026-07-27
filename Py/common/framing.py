import json, struct
import os, sys, socket
from pathlib import Path

from Py.common.protocol import Status


class Frame:
    @staticmethod
    def __recvExact(sock: socket.socket, n: int) -> bytes:
        data = bytearray()
        while len(data) < n:
            chunk = sock.recv(n - len(data))
            if not chunk:
                raise OSError("Socket closed.")
            data.extend(chunk)
        return bytes(data)

    @staticmethod
    def __createRequestPayload(data: dict, format: str = ">I", encoding: str = "utf-8"):
        """
        Big-endian unsigned 4-byte length + JSON header.
        If data contains a reserved key '_payload', it will be sent as raw bytes
        after the JSON header, and 'payload_len' will be inserted into the header.
        """

        payload = data.get("_payload", b"")

        if payload is None:
            payload = b""
        if isinstance(payload, str):
            payload = payload.encode(encoding)
        if not isinstance(payload, (bytes, bytearray, memoryview)):
            raise TypeError("_payload must be byte-like !")

        header = dict(data)
        header.pop("_payload", None)

        if payload:
            header["payload_len"] = len(payload)

        request = json.dumps(header).encode(encoding)
        payload_len = struct.pack(format, len(payload))
        send_payload = payload_len + request + bytes(payload)

        return send_payload

    @staticmethod
    def send(sock: socket.socket, data) -> int:
        try:
            send_payload = Frame.__createRequestPayload(data)
            sock.sendall(send_payload)
            return Status.SUCCESS
        except OSError:
            return Status.FAILED

    @staticmethod
    def read(
        sock: socket.socket, format: str = ">I", encoding: str = "utf-8"
    ) -> int | dict:
        try:
            hdr_len_bytes = Frame.__recvExact(sock, 4)
            hdr_len = struct.unpack(format, hdr_len_bytes)[0]

            if hdr_len == 0 or hdr_len > 10 * 1024 * 1024:
                return Status.FAILED

            hdr_bytes = Frame.__recvExact(sock, hdr_len)
            header = json.loads(hdr_bytes.decode(encoding))

            payload_len = int(header.get("payload_len", 0))

            if payload_len != 0:
                payload = Frame.__recvExact(sock, payload_len)
            else:
                payload = b""

            return {"header": header, "payload": payload}

        except (OSError, ValueError, json.JSONDecodeError, UnicodeDecodeError):
            return Status.FAILED
