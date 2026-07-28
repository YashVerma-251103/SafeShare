import socket, threading, os, sys
from dataclasses import dataclass

from common.framing import Frame
from common.protocol import *
from common.state import GlobalState
from tokenMngr import TokenInfo, TokenMnger
from fileMngr import FileMngr
from chatMngr import ChatMngr


@dataclass
class ReqContext:
    client_sock: object
    header: dict
    payload: bytes
    peer_ip: str


class Server:
    def __init__(self, port, shared_root):
        self.port = port
        self.shared_root = shared_root
        self.tokens = TokenMnger("./safeshare.db")
        self.files = FileMngr(shared_root)
        self.chat = ChatMngr()
        self.console_mtx = threading.Lock()

    def run(self):
        srv = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)

        try:
            srv.setsockopt(
                level=socket.SOL_SOCKET, optname=socket.SO_REUSEADDR, value=1
            )

            addr = (IPs.ALL_INTERFACE_IP, self.port)
            srv.bind(addr)

            srv.listen(10)

            print(f"[Server] listening on port {self.port} !")

            while True:
                client_sock, (peer_ip, peer_port) = srv.accept()

                t = threading.Thread(
                    target=self.handleClient, args=(client_sock, peer_ip), daemon=True
                )

                t.start()

        finally:
            srv.close()

    def handleClient(self, client_sock: socket.socket, client_ip: str):
        try:
            while True:
                frame = Frame.read(client_sock)
                if Status.FAILED == frame:
                    break

                hdr = frame["header"]
                pyld = frame["payload"]
                msg_type = hdr.get("type", "")

                hndlrs = {
                    MSG.PERM_REQ: self.__handle_PERM_REQ,
                    MSG.LIST_REQ: self.__handle_LIST_REQ,
                    MSG.DOWNLOAD_REQ: self.__handle_DOWNLOAD_REQ,
                    MSG.CHAT_SEND: self.__handle_CHAT_SEND,
                }

                hndlr = hndlrs.get(msg_type)

                if hndlr is None:
                    Frame.send(
                        client_sock, {"type": MSG.ERROR, "message": MSG.UNKNOWN}
                    )

                else:
                    context = ReqContext(
                        client_sock=client_sock,
                        header=hdr,
                        payload=pyld,
                        peer_ip=client_ip,
                    )
                    hndlr(context)
        finally:
            client_sock.close()

    def __handle_PERM_REQ(self, Context: ReqContext):
        client_sock = Context.client_sock
        peer_ip = Context.peer_ip
        header = Context.header
        from_device = header.get("from_device", "unknown")
        display = header.get("display_name", "<No_Name>")
        reason = header.get("reason", "")

        print(f"[Server] pairing request from {display} ({peer_ip})")

        accepted = GlobalState.get().waitForApproval(f"{display} ({peer_ip})", reason)

        resp = {"type": MSG.PERM_RESP, "id": header.get("id", "")}

        if accepted:
            token = self.tokens.issueToken(
                device=from_device,
                client_ip=peer_ip,
                ttl_secs=Times.DEFAULT_TTL,
                persistent=False,
            )
            resp["status"] = Status.ACCEPT
            resp["token"] = token
            resp["ttl"] = Times.DEFAULT_TTL
        else:
            resp["status"] = Status.DENY

        Frame.send(sock=client_sock, data=resp)

    def __handle_LIST_REQ(self, Context: ReqContext):
        client_sock = Context.client_sock
        header = Context.header
        peer_ip = Context.peer_ip

        token = header.get("token")
        path = header.get("path", ".")

        resp = {"type": MSG.LIST_RESP, "id": header.get("id", "")}

        if (token is None) or (not self.tokens.validateToken(token, peer_ip)):
            resp["status"] = Status.ERROR
            resp["message"] = "UNAUTHORIZED OR INVALID SESSION"
            Frame.send(client_sock, resp)
            return

        entries = self.files.list(path)
        resp["status"] = Status.OK
        resp["entries"] = entries
        Frame.send(client_sock, resp)

    def __handle_DOWNLOAD_REQ(self, Context: ReqContext):
        client_sock = Context.client_sock
        header = Context.header
        peer_ip = Context.peer_ip

        token = header.get("token")
        filename = header.get("path", "")

        if (token is None) or (not self.tokens.validate_token(token, peer_ip)):
            Frame.send(client_sock, {
                "type": MSG.ERROR,
                "message": MSG.UNAUTH
            })
            return

        if not self.files.exists(filename):
            Frame.send(client_sock, {
                "type": MSG.ERROR,
                "message": "NOT_FOUND"
            })
            return

        accepted = GlobalState.get().wait_for_approval(
            f"Download: {filename}",
            f"Request from {peer_ip}",
        )

        if not accepted:
            Frame.send(client_sock, {
                "type": MSG.ERROR,
                "message": "ACCESS_DENIED_BY_HOST"
            })
            return

        file_obj = self.files.open_read(filename)
        if file_obj is None:
            Frame.send(client_sock, {
                "type": MSG.ERROR,
                "message": "OPEN_FAILED"
            })
            return

        with file_obj:
            Frame.send(client_sock, {
                "type": MSG.DOWNLOAD_RESP,
                "filename": filename
            })

            while True:
                chunk = file_obj.read(64 * 1024)
                if not chunk:
                    break

                Frame.send(client_sock, {
                    "type": MSG.FILE_CHUNK,
                    "payload_len": len(chunk),
                    "_payload": chunk
                })

            Frame.send(client_sock, {
                "type": MSG.TRANSFER_END
            })

    def __handle_CHAT_SEND(self, Context: ReqContext):
        client_sock = Context.client_sock
        header = Context.header
        payload = Context.payload
        peer_ip = Context.peer_ip

        token = header.get("token")

        if (token is None) or (not self.tokens.validateToken(token,peer_ip)):
            Frame.send(client_sock,{
                "type":MSG.ERROR,
                "message" : MSG.UNAUTH
            })
            return

        # decode and process chat payload here
        # message_text = payload.decode("utf-8", errors="replace")
        # self.chat.broadcast(...)
        Frame.send(client_sock, {
            "type": "MSG_OK"
        })
