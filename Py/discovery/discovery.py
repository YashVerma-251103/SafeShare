class Discovery:

    @staticmethod
    def startAnnouncer(name:str,port:int)->None:
        pass

class DiscoveredPeer:
    def __init__(self,device_id:str,name:str,ip:str,port:int):
        self.device_id = device_id
        self.name = name
        self.ip = ip
        self.port = port

    