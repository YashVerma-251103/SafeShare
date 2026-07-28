import threading, json, struct, socket, time
from typing import List

from common.protocol import *
from common.framing import Frame

class DiscoveredPeer:
    def __init__(self, device_id: str = "", name: str = "", ip: str ="", port: int = Ports.DEFAULT):
        self.device_id = device_id
        self.name = name
        self.ip = ip
        self.port = port

class Discovery:


    @staticmethod
    def startAnnouncer(name: str, service_port: int) -> None:

        def run():
            BROADCAST = 1
            sock = socket.socket(
                family= socket.AF_INET,
                type= socket.SOCK_DGRAM # UDP Socket
            )

            try:
                sock.setsockopt(
                    level = socket.SOL_SOCKET,
                    optname = socket.SO_BROADCAST,
                    value = BROADCAST
                )

                addr = (IPs.BROADCAST_IP, Ports.DISCOVERY)

                while True:
                    j = {
                        "type" : MSG.ANNOUNCE,
                        "device_id" : name + "-id",
                        "name" : name,
                        "port" : service_port
                    }

                    data = json.dumps(j).encode("utf-8")
                    sock.sendto(data,addr)
                    time.sleep(3) # Here it is 3 Seconds.
            finally:
                sock.close()

        t = threading.Thread(target=run,daemon=True)
        t.start()
        return t
    
    @staticmethod
    def scanOnce(listen_ms: int = Times.DEFAULT_LISTEN_TIME) -> List[DiscoveredPeer]:

        res = list()
        buff_size = 8192

        sock = socket.socket(
            family= socket.AF_INET,
            type=socket.SOCK_DGRAM,
        )

        sock.setsockopt(
            level = socket.SOL_SOCKET,
            optname = socket.SO_REUSEADDR,
            value = 1
        )

        addr = (IPs.ALL_INTERFACE_IP,Ports.DISCOVERY)

        sock.bind(addr)

        sock.settimeout(listen_ms/1000.0)

        while True:
            try : 
                data, src = sock.recvfrom(buff_size)

            except socket.timeout:
                break

            if not data:
                break

            try:
                text = data.decode("utf-8")
                j = json.loads(text)

                if "type" in j and j["type"] == MSG.ANNOUNCE:
                    p = DiscoveredPeer()
                    p.device_id = j.get("device_id","")
                    p.name = j.get("name","")
                    p.port = j.get("port",Ports.DEFAULT)
                    p.ip = src[0]
                    res.append(p)

            except Exception:
                pass

        sock.close()
        return res
