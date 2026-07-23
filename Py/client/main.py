
from typing import List
import client

def showUsage()->None:
    pass
def discoverPeers()->None:
    pass
def request(args:List)->int:
    pass
def download(args:List)->int:
    pass
def listFiles(args:List)->int:
    pass

def driver(args:List)->int:
    if (len(args)<2):
        showUsage()
        return status_OK

    cmd = args[1]

    if DISCOVER_CMD == cmd:
        discoverPeers()
        return status_OK

    elif REQUEST_CMD == cmd:
        status = request(args)
        return status

    elif DOWNLOAD_CMD == cmd:
        status = download(args)
        return status

    elif LIST_CMD == cmd:
        status = listFiles(args)
        return status

    else:
        print("Reached Else part")
        return status_OK

if __name__ == "__main__":
    pass

