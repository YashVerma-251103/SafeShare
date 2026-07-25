from enum import Enum

class Commands(Enum):
    DISCOVER = "DISCOVER"
    REQUEST = "REQUEST"
    DOWNLOAD = "DOWNLOAD"
    LIST = "LIST"

class Status(Enum):
    RESPONSE_OK = "OK"
    OK = 0
    ERROR = 1
    FAILED = 2
    SUCCESS = 3
    ACCEPT = 4
        

        