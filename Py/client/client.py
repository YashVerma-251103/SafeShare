from typing import List
import json, socket, struct

from common.Enums import Status

def __createRequestPayload(data:dict,format:str=">I"):# Big-Endian Unsigned 4-byte (32 bit) integer -- uint32_t
    request = json.dump(data).encode()
    payload_len = struct.pack(format, len(request))
    send_payload = payload_len + request
    return send_payload


def __sendFrame(sock, data) -> int:
    try:
        send_payload = __createRequestPayload(data)
        sock.sendall(send_payload)
        return Status.SUCCESS
    except OSError:
        return Status.FAILED

def __readFrame(sock,format:str=">I") -> int | dict:
    try:
        recv_payload_len = struct.unpack(format,sock.recv(4))[0]
        response = json.load(sock.recv(recv_payload_len))
        return response
    except OSError:
        return Status.FAILED


class Client:
    @staticmethod
    def listRemoteFiles(ip:str,port:int,token:str,need_return:bool=False)->List[str] | None:
        files = None
    
        with socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM, proto=0) as sock:
            sock.connect((ip,port))
    
            send_data = {
                "type" : "MSG_LIST",
                "token" : token,
                "path" : "."
            }
            status = __sendFrame(sock,send_data)
            if (Status.FAILED == status):
                print("Could Not send the payload. Aborting !")
                
            else:
                response = __readFrame(sock)
                if (Status.FAILED == response):
                    print("Could not recieve the response. Aborting !")
                    
                else:
                    if Status.RESPONSE_OK == response["status"]:
                        if need_return:
                            files = response["entries"]
                        else:
                            print("--- Remote Files ---")
                            for file in response["entries"]: 
                                print(f" - {file}")
                    else:
                        print(f"Error: {response.get("message", "Unknown")}")

        return files

            
    @staticmethod
    def sendPermRequest(ip:str,port:int,display_name:str,reason:str) -> str:
        token = None

        with socket.socket(family=socket.AF_INET,type=socket.SOCK_STREAM,proto=0) as sock:
            sock.connect((ip,port))

            send_data = {
                "type" : "MSG_PERM_REQUEST",
                "display_name" : display_name,
                "id" : display_name + "-req",
                "from_device" : display_name + "-dev",
                "reason" : reason
            }

            status = __sendFrame(sock,send_data)
            if (Status.FAILED == status):
                print("Could Not send the payload. Aborting !")
        
            else:
                response = __readFrame(sock)
                if (Status.FAILED == response):
                    print("Could not recieve the response. Aborting !")

                else:
                    hdr,payload = __readFrame(sock)
                    if (hdr["status"] == Status.ACCEPT):
                        token = hdr["token"]

        return token


    @staticmethod
    def downloadFile(ip:str,port:int,token:str,remote_path:str,local_path:str) -> str:
        result = None

        with socket.socket(family=socket.AF_INET,type=socket.SOCK_STREAM,proto=0) as sock:
            sock.connect((ip,port))

            send_data = {
                "type": "MSG_DOWNLOAD_REQ",
                "token":token,
                "path" : remote_path
            }
            status = __sendFrame(sock,send_data)
            if (Status.FAILED == status):
                print("Could Not send the payload. Aborting !")
            else:
                pass

        return result