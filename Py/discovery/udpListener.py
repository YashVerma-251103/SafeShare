from typing import List

from discovery.discovery import DiscoveredPeer

class Listener:
    
    @staticmethod
    def scanOnce(listen_ms : int) -> List[DiscoveredPeer]:
        pass