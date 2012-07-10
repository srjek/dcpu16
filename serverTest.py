##server.py
from socket import *      #import the socket library
 
##let's set up some constants
HOST = ''    #we are the host
PORT = 29876    #arbitrary port not currently in use
ADDR = (HOST,PORT)    #we need a tuple for the address
BUFSIZE = 4096    #reasonably sized buffer for data
 
## now we create a new socket object (serv)
## see the python docs for more information on the socket types/flags
serv = socket(AF_INET,SOCK_STREAM)    
 
##bind our socket to the address
serv.bind((ADDR))    #the double parens are to create a tuple with one element
serv.listen(5)    #5 is the maximum number of queued connections we'll allow
print("Listening...")


while True:
    conn,addr = serv.accept() #accept the connection
    print("Received connection from",addr[0],"on port",addr[1])
    conn.recv(4096)
    conn.send(b'HTTP/1.0 200 OK\n')
    conn.send(b'Content-Type: text/text\n\n')
    conn.send(b'TEST\n')
    conn.close()

serv.close()
