import pygame, sys, os
from pygame.locals import *
from multiprocessing import Value, Queue
import queue

emulatorRoot = ""
if __name__ == '__main__':
    emulatorRoot = os.path.dirname(os.path.realpath(sys.argv[0]))
else:
    emulatorRoot = os.path.dirname(os.path.realpath(__file__))
emulatorRoot = os.path.abspath(emulatorRoot)
sys.path.insert(0, os.path.join(emulatorRoot, "core"))

def getCPUs():
    cpus = {}
    cpuRoot = os.path.join(emulatorRoot, "cpus")
    files = os.listdir(cpuRoot)
    for name in files:
        path = os.path.join(cpuRoot, name)
        if name.startswith("_"):
            continue
        elif os.path.isdir(path):
            pass
        elif path.endswith(".py") and os.path.isfile(path):
            name = name[:-3]
        else:
            continue
        if name in cpus:
            print("Conflicting cpu names ("+repr(name)+")", file=sys.stderr)
            cpus[name] = None
        else:
            cpus[name] = path
    for name in list(cpus.keys()):
        if cpus[name] == None:
            cpus.pop(name)
    return cpus
def getDevices():
    devices = {}
    deviceRoot = os.path.join(emulatorRoot, "devices")
    files = os.listdir(deviceRoot)
    for name in files:
        path = os.path.join(deviceRoot, name)
        if name.startswith("_"):
            continue
        elif os.path.isdir(path):
            pass
        elif path.endswith(".py") and os.path.isfile(path):
            name = name[:-3]
        else:
            continue
        if name in devices:
            print("Conflicting device names ("+repr(name)+")", file=sys.stderr)
            devices[name] = None
        else:
            devices[name] = path
    for name in list(devices.keys()):
        if devices[name] == None:
            devices.pop(name)
    return devices
def parseCmdlineArguments():
    import argparse
    class maintainOrder(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            if not 'ordered_args' in namespace:
                setattr(namespace, 'ordered_args', [])
            previous = namespace.ordered_args
            previous.append((self.dest, values))
            setattr(namespace, 'ordered_args', previous)
        
    cpus = getCPUs()#["dcpu16"]
    devices = getDevices()#["LEM1802", "genericKeyboard", "genericClock", "M35FD", "SPED3"]
    
    parser = argparse.ArgumentParser(description="Emulates systems and accessories from 0x10c. Any devices specified attach to the last cpu before.")
    parser.add_argument("--image", nargs=1, action=maintainOrder, help="Disables cpu boot sequence, preloads [image] into memory")
    for cpu in cpus:
        parser.add_argument("--"+cpu, nargs=0, action=maintainOrder, help="Creates a "+cpu+".")
    for dev in devices:
        parser.add_argument("--"+dev, nargs=0, action=maintainOrder, help="Creates a "+dev+".")
    namespace = parser.parse_args()
    args = ()
    if 'ordered_args' in namespace:
        args = namespace.ordered_args
    print(args)
    
    configuration = []
    defaultCpu = ("dcpu16",)
    curCpu = "default"
    curHardware = []
    curImage = None
    
    for cmd, opt in args:
        if cmd in cpus:
            if not ((curCpu == "default") and (len(curHardware) == 0)):
                if curCpu == "default": curCpu = defaultCpu
                configuration.append((curCpu, curImage, tuple(curHardware)))
            tmp = [cmd]
            tmp.extend(opt)
            curCpu = tuple(tmp)
            curHardware = []
            curImage=None
        elif cmd in devices:
            tmp = [cmd]
            tmp.extend(opt)
            curHardware.append(tuple(tmp))
        elif cmd == "image":
            curImage = os.path.abspath(opt[0])
    if curCpu == "default": curCpu = defaultCpu
    configuration.append((curCpu, curImage, tuple(curHardware)))
    
    print(tuple(configuration))
    return tuple(configuration)

class rootGui:
    def __init__(self):
        self.callbackQueue = queue.Queue()
        self.quitQueue = queue.Queue()
    def callback(self, function, args):
        self.callbackQueue.put((function, args))
    def registerQuit(self, function, args):
        self.quitQueue.put((function, args))
        
    def run(self, checkQueue):
        import tkinter
        from tkinter import ttk
        self.quitting = False
        root = tkinter.Tk()

        def cycle():
            checkQueue()
            if self.quitting:
                return
            while True:
                try:
                    (function, args) = self.callbackQueue.get_nowait()
                    function(*args)
                except queue.Empty:
                    root.after(1000, cycle)
                    return
        def quit():
            self.quiting = True
            while True:
                try:
                    (function, args) = self.quitQueue.get_nowait()
                    function(*args)
                except queue.Empty:
                    break
            root.quit()
        ttk.Button(root, text="Quit", command=quit).grid(column=0, row=0, sticky=(tkinter.W, tkinter.E))
            
        root.after(1000, cycle)
        root.mainloop()
        self.quitting = True
        while True:
            try:
                (function, args) = self.quitQueue.get_nowait()
                function(*args)
            except queue.Empty:
                break
        

def main():
    import time
    initTime = time.time()
    
    cpuPath = getCPUs()
    devicePath = getDevices()
    configuration = parseCmdlineArguments()
    os.chdir(emulatorRoot)  #some cmdline arguments depend on cwd, so wait until now to move
    
    gui = rootGui()
    running = Value("i", 1, lock=False)
    error = Queue(200)
    comp = []
    
    import imp
    for cpu, imagePath, deviceConfig in configuration:
        name = cpu[0]
        args = cpu[1:]
        path = cpuPath[name]
        if path.endswith(".py"):
            path = os.path.dirname(path)
        
        module = None
        try:
            tmp = imp.find_module(name, [path,])
            module = imp.load_module(name, *tmp)
        except ImportError:
            print("Failed to load cpu "+repr(name),file=sys.stderr)
            if tmp[0] != None: tmp[0].close()
            continue
        if tmp[0] != None: tmp[0].close()
        cpu_class = module.main
        comp.append(cpu_class(running, error, args, imagePath, devicePath, deviceConfig))
    
    for cpu in comp:
        cpu.start()
        endTime = time.time() + 0.25
        while endTime > time.time():
            pass
    
    print("Initialization took "+str(round(time.time()-initTime, 3))+" seconds")
    
    def checkQueue():
        while True:
            try:
                e = error.get_nowait()
                for line in e:
                    print(line, file=sys.stderr)
            except queue.Empty:
                break
    
    gui.run(checkQueue)
    running.value = False
    for cpu in comp:
        cpu.finishUp()
        checkQueue()

if __name__ == '__main__':
    main()
