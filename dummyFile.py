class dummyFile:
    def __init__(self):
        self.errors = "unknown"
        self.buffer = ""
    def write(self, x):
        self.buffer += x
    def flush(self):
        dummy = 42
