from typing import List
import json, socket, struct

from Py.common.protocol import Status, MSG
from common.framing import *


class Client:
    @staticmethod
    def listRemoteFiles(
        ip: str, port: int, token: str, need_return: bool = False
    ) -> List[str] | None:
        files = None
        try:
            with socket.socket(
                family=socket.AF_INET, type=socket.SOCK_STREAM, proto=0
            ) as sock:
                sock.connect((ip, port))

                send_data = {"type": MSG.LIST_REQ, "token": token, "path": "."}
                status = Frame.send(sock, send_data)
                if Status.FAILED == status:
                    print("Could Not send the payload. Aborting !")
                    return None

                response = Frame.read(sock)
                if Status.FAILED == response:
                    print("Could not recieve the response. Aborting !")
                    return None

                header = response["header"]
                if Status.RESPONSE_OK == header.get("status"):
                    if need_return:
                        files = header.get("entries", [])
                    else:
                        print("--- Remote Files ---")
                        for file in header.get("entries", []):
                            print(f" - {file}")
                else:
                    print(f"Error: {response.get("message", "Unknown")}")

        except OSError as e:
            print(f"Connection Error : {e}")

        return files

    @staticmethod
    def sendPermRequest(ip: str, port: int, display_name: str, reason: str) -> str:
        token = ""

        try:
            with socket.socket(
                family=socket.AF_INET, type=socket.SOCK_STREAM, proto=0
            ) as sock:
                sock.connect((ip, port))

                send_data = {
                    "type": MSG.PERM_REQ,
                    "display_name": display_name,
                    "id": display_name + "-req",
                    "from_device": display_name + "-dev",
                    "reason": reason,
                }

                status = Frame.send(sock, send_data)
                if Status.FAILED == status:
                    print("Could Not send the payload. Aborting !")
                    return ""

                response = Frame.read(sock)
                if Status.FAILED == response:
                    print("Could not recieve the response. Aborting !")
                    return ""

                hdr = response["header"]
                if Status.ACCEPT == hdr.get("status"):
                    token = hdr.get("token", "")

        except OSError as e:
            print(f"Connection Error : {e}")

        return token

    @staticmethod
    def downloadFile(
        ip: str, port: int, token: str, remote_path: str, local_path: str
    ) -> str:
        result = None

        try:
            with socket.socket(
                family=socket.AF_INET, type=socket.SOCK_STREAM, proto=0
            ) as sock:
                sock.connect((ip, port))

                send_data = {
                    "type": MSG.DOWNLOAD_REQ,
                    "token": token,
                    "path": remote_path,
                }

                status = Frame.send(sock, send_data)
                if Status.FAILED == status:
                    print("Could Not send the payload. Aborting !")
                    return "Connection Failed."

                try:
                    outfile = open(local_path, "wb")
                except OSError:
                    return f"File Write Error : {local_path}"

                result = f"Download Complete : {local_path}"

                try:
                    while True:
                        response = Frame.read(sock)
                        if Status.FAILED == response:
                            result = "Connection Lost."

                            try:
                                outfile.close()
                            finally:
                                if os.path.exists(local_path):
                                    os.remove(local_path)
                            break

                        header = response["header"]
                        payload = response["payload"]
                        msg_type = header.get("type", "")

                        if MSG.FILE_CHUNK == msg_type:
                            if payload:
                                outfile.write(payload)

                        elif MSG.TRANSFER_END == msg_type:
                            outfile.close()
                            break

                        elif MSG.ERROR == msg_type:
                            result = f"Error : {header.get("message","Unknown")}"
                            outfile.close()
                            if os.path.exists(local_path):
                                os.remove(local_path)
                            break
                finally:
                    if not outfile.closed:
                        outfile.close()

                return result
        except OSError as e:
            return f"Connection Failed: {e}"
