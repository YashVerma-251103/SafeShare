import argparse, sys
import json, socket, struct
from typing import List

from Py.common.protocol import Commands, Status
from discovery.udpListener import Listener
from client.client import Client


def showUsage() -> None:
    print(
        "Usage : safeshare-client discover | request <ip> <port> <display_name> <reasons>"
    )


def discoverPeers(args: dict) -> None:
    ms: int = args["ms"] if "ms" in args else 800
    peers = Listener.scanOnce(ms)
    print(f"Found {len(peers)} peers:")
    for p in peers:
        print(f"{p.name} ({p.device_id}) @ {p.ip} : {p.port}")


def request(args: dict) -> None:

    if len(args) != 5:
        print("Usage: safeshare-client request <ip> <port> <display_name> <reason>")

    ip = args["ip"]
    port = args["port"]
    display = args["display"]
    reason = args["reason"]
    Client.sendPermRequest(ip, port, display, reason)


def listFiles(args: dict, need_return: bool = False) -> List[str] | None:
    if len(args) != 4:
        print("Usage: safeshare-client list <ip> <port> <token>")
        return None

    ip = args["ip"]
    port = args["port"]
    token = args["token"]

    files = Client.listRemoteFiles(ip, port, token, need_return)

    return files


def download(args: dict) -> None:
    if len(args) != 6:
        print(
            "Usage: safeshare-client download <ip> <port> <token> <remote_path> <local_path>"
        )
        return

    ip = args["ip"]
    port = args["port"]
    token = args["token"]
    r_path = args["r_path"]
    l_path = args["l_path"]

    Client.downloadFile(ip, port, token, r_path, l_path)


def driver(args: dict) -> None:
    if "cmd" not in args:
        showUsage()
        return None

    cmd = args["cmd"]

    if Commands.DISCOVER == cmd:
        discoverPeers(args)
    elif Commands.REQUEST == cmd:
        request(args)
    elif Commands.DOWNLOAD == cmd:
        download(args)
    elif Commands.LIST == cmd:
        listFiles(args)
    else:
        print("Reached else part")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    all_args = {
        "Cmd": ["--cmd", str, "Operation you want to perform."],
        "Ip": ["--ip", str, "Machine IP you want to connect."],
        "Port": ["--port", int, "Corresponding IP Port you want to connect to."],
        "Display Name": ["--display", str, "Your Machine Display Name."],
        "Reasons": ["--reason", str, "Reason for Connection with other Party."],
        "Listen Time": ["--ms", int, "Time for listener scan in ms."],
        "Auth Token": ["--token", str, "Auth Token recieved."],
        "Remote Path": ["--r_path", str, "Remote Path"],
        "Local Path": ["--l_path", str, "Local Path"],
    }

    for params in all_args.values():
        ap.add_argument(params[0], type=params[1], help=params[2])

    args = ap.parse_args()

    args_dict = {k: v for k, v in vars(args).items() if v is not None}

    driver(args=args_dict)
