from server.server import Server
from discovery.discovery import Discovery
from py_ui.mini_web import MiniWeb

import threading
from pathlib import Path
import argparse


def initialize():
	ap = argparse.ArgumentParser(
				description = __doc__,
				formatter_class=argparse.RawDescriptionHelpFormatter
		)
	ap.add_argument("-p","--port",default=55001,type=int,help="Assign Server Port")
	ap.add_argument("-sd","--shared_dir", type=Path, help="Absolute Path of the shared directory")

	args = ap.parse_args()
	
	port = args.port

	if args.shared_dir and Path.is_dir(args.shared_dir):
		shared_dir = args.shared_dir
	else:
		print("Going with the Default Shared directory!")
		default_shared_dir_name = "Shared"
		shared_dir = Path(Path(__file__).parent.parent.resolve(),default_shared_dir_name)
	
	Path.mkdir(shared_dir,exist_ok=True)

	return (shared_dir, port)



if __name__ == "__main__":
	
	shared_dir, port = initialize()

	print(
		"----------------------------------",
		"SafeShare Node Starting...",
		f"Sharing Folder: {shared_dir}",
		"----------------------------------",
		sep='\n',end=''
	)

	Discovery.startAnnouncer("SafeShare-WebNode",port)

	server = Server(port,shared_dir)
	server_thread = threading.Thread(target=server.run)
	server_thread.start()

	MiniWeb.start_web_server()