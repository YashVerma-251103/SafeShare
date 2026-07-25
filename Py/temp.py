import argparse
from pathlib import Path
# print(Path(__file__))



ap = argparse.ArgumentParser(
            description = __doc__,
            formatter_class=argparse.RawDescriptionHelpFormatter
    )
ap.add_argument("-p","--port",default=55001,type=int,help="Assign Server Port")
ap.add_argument("-sd","--shared_dir", type=Path, help="Path of the shared directory")

args = ap.parse_args()

port = args.port
s = args.shared_dir
print(port,s,args.shared_dir,type(s),type(args))



default="shared"