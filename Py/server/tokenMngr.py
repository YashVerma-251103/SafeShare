import threading, secrets, time, sys


def now_ms():
    return int(time.time() * 1000)


def random_hex(n_bytes):
    return secrets.token_hex(n_bytes)


class TokenInfo:
    def __init__(
        self,
        token: str,
        device: str,
        client_ip: str,
        expires_ms: int,
        persistent: bool = False,
    ):
        self.token = token
        self.device = device
        self.client_ip = client_ip
        self.expires_ms = expires_ms
        self.persistent = persistent


class TokenMnger:
    def __init__(self, dbpath):
        self.mtx = threading.Lock()
        self.tokens = dict()
        self.dbpath = dbpath

    def issueToken(self, device, client_ip, ttl_secs, persistent: bool = False):
        with self.mtx:
            t = random_hex(20)

            expires_ms = now_ms() + int(ttl_secs)*1000
            ti = TokenInfo(t,device,client_ip,expires_ms,persistent)

            self.tokens[t]=ti

            print(f"[Token] issued {t} for {device} @ {client_ip} ttl={ttl_secs}s")

            return t

        
    def validateToken(self,token,requesting_ip):
        with self.mtx :
            ti = self.tokens.get(token)

            if ti is None:
                return False

            if now_ms() > ti.expires_ms:
                del self.tokens[token]
                return False

            if ti.client_ip != requesting_ip:
                print(f"[Security] IP mismatch ! Token ownner: {ti.client_ip}, Requestor: {requesting_ip}")
                return False

            return True

    def revokeToken(self,token):
        with self.mtx:
            self.tokens.pop(token,None)