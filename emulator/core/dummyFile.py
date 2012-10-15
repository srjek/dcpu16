class dummyFile:
    def __init__(self):
        self.errors = "unknown"
        self.buffer = ""
    def write(self, x):
        self.buffer += x
    def flush(self):
        dummy = 42

class queueFile:
    def __init__(self, queue):
        self.errors = "unknown"
        self.buffer = ""
        self.queue = queue
    def write(self, x):
        self.buffer += x
        if "\n" in self.buffer:
            tmp = self.buffer.split("\n")
            self.buffer = tmp[-1]
            self.queue.put(tmp[:-1])
                
    def flush(self):
        dummy = 42
