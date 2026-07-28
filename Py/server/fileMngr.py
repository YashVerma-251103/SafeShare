import os

class FileMngr:

    def __init__(self, shared_root):
        self.root = os.path.realpath(shared_root)
        os.makedirs(self.root,exist_ok=True)

    def __resolveSafe(self,path):
        # join Root + User Path, then resolve Symlinks -> / .. 
        candidate = os.path.realpath(os.path.join(self.root,path))

        # Enforce Sandbox : candidate must be under root.
        root_prefix = self.root.rstrip(os.sep) + os.sep

        if candidate != self.root and not candidate.startswith(root_prefix):
            return None

        return candidate

    def list(self,path):
        p = self.__resolveSafe(path)
        if p is None or not os.path.isdir(p):
            return list()

        try:
            return os.listdir(p)

        except OSError:
            return list()

    def exists(self,path):
        p = self.__resolveSafe(path)

        if p is None:
            return False

        return os.path.exists(p)

    def openRead(self,path):
        p = self.__resolveSafe(path)

        if p is None or not os.path.isfile(p):
            return None

        return open(p, "r", encoding="utf-8")