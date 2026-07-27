import threading, secrets
from typing import Dict, List

from common.protocol import Status
from discovery.discovery import DiscoveredPeer


class PendingReq:
    def __init__(
        self,
        id: str,
        name: str,
        reason: str = "",
        decision: Status = Status.WAITING,
    ):
        self.id: str = id
        self.name: str = name
        self.reason: str = reason
        self.decision: Status = decision


class GlobalState:
    __instance = None
    __boot_lock = threading.Lock()

    def __new__(cls):
        # Singleton
        if cls.__instance is None:
            with cls.__boot_lock:
                if cls.__instance is None:
                    cls.__instance = super().__new__(cls)
                    cls.__instance.__initOnce()
        return cls.__instance

    @classmethod
    def get(cls):
        return cls()

    def __initOnce(self):
        self.mtx = threading.Lock()
        self.cond_var = threading.Condition(self.mtx)
        self.requests = Dict[str:PendingReq]  # req_id -> PendingReq
        self.peer_map = Dict[str:DiscoveredPeer]  # ip -> DiscoveredPeer



    """Server: Approval Logic"""
    def waitForApproval(self, name: str, reason: str = ""):
        req_id = secrets.token_hex(8)

        with self.cond_var:
            self.requests[req_id] = PendingReq(req_id, name, reason)
            self.cond_var.wait_for(
                lambda: self.requests.get(req_id) is not None
                and self.requests[req_id].decision != Status.WAITING
            )

            accepted = (self.requests[req_id].decision == Status.ACCEPT)

            del self.requests[req_id]

            return accepted

    def submitDecision(self,req_id : str, accept:bool):
        with self.cond_var:
            req = self.requests.get(req_id)
            if req_id is not None:
                req.decision = Status.ACCEPT if accept else Status.DENY
                self.cond_var.notify_all()

    def getPending(self):
        with self.cond_var:
            return list(self.requests.values())


    """Client : Persistent Discovery"""
    def mergePeers(self,new_peers:List[DiscoveredPeer]):
        with self.mtx:
            for p in new_peers:
                self.peer_map[p.ip] = p

    def removePeer(self, ip: str):
        with self.mtx:
            self.peer_map.pop(ip,None)

    def getAllPeers(self):
        with self.mtx:
            return list(self.peer_map.values())