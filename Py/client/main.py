import argparse, sys
import json, socket, struct
from typing import List

from common.Enums import Commands,Status
from discovery.udpListener import Listener
from client.client import Client


def showUsage()->None:
    print("Usage : safeshare-client discover | request <ip> <port> <display_name> <reasons>")

def discoverPeers()->None:
    peers = Listener.scanOnce(800)
    print(f"Found {len(peers)} peers:")
    for p in peers:
        print(f"{p.name} ({p.device_id}) @ {p.ip} : {p.port}")  
    
def request(args:dict)->None:

    if (len(args) != 5):
        print("Usage: safeshare-client request <ip> <port> <display_name> <reason>")
        
    ip = args["ip"]
    port = args["port"]
    display = args["display"]
    reason = args["reason"]
    Client.sendPermRequest(ip, port, display, reason)
    
def listFiles(args:dict,need_return:bool=False)-> List[str] | None:
    if (len(args) != 4):
        print("Usage: safeshare-client list <ip> <port> <token>")
        return None

    ip = args["ip"]
    port = args["port"]
    token = args["token"]

    files = Client.listRemoteFiles(ip,port,token,need_return)

    return files

def download(args:dict)->None:
    if len(args) != 6:
        print("Usage: safeshare-client download <ip> <port> <token> <remote_path> <local_path>")
        return

    ip = args["ip"]
    port = args["port"]
    token = args["token"]
    r_path = args["r_path"]
    l_path = args["l_path"]

    Client.downloadFile(ip,port,token,r_path,l_path)

def driver(args:dict)->None:
    if "cmd" not in args:
        showUsage()
        return None

    cmd = args["cmd"]

    if Commands.DISCOVER == cmd: discoverPeers()
    elif Commands.REQUEST == cmd: request(args)
    elif Commands.DOWNLOAD == cmd: download(args)
    elif Commands.LIST == cmd: listFiles(args)
    else: print("Reached else part")

if __name__ == "__main__":
    pass

