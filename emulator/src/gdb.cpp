#include "gdb.h"

//#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

struct async_fillBufferCallback {
private:
    gdb_remote* gdb;
  
public:
    async_fillBufferCallback(gdb_remote* gdb) : gdb(gdb) {}
    void operator()(const boost::system::error_code& error, size_t bytes_transferred) {
        gdb->async_fillBufferFinished(error, bytes_transferred);
    }

};
    
int gdb_remote::peekChar() {
    if (buf_size == 0) {
        buf_pos = 0;
        buf_size = socket->receive(buffer(buf, BUFFER_SIZE));
    }
    if (buf_size == 0)
        return EOF;
    
    return buf[buf_pos];
}
int gdb_remote::readChar() {
    if (peekChar() == EOF)
        return EOF;
    buf_size--;
    return buf[buf_pos++];
}
void gdb_remote::putChar(char c) {
    socket->send(buffer(&c, 1));
}
void gdb_remote::putData(char* c, int size) {
    socket->send(buffer(c, size));
}
void gdb_remote::async_fillBufferFinished(const boost::system::error_code& error, size_t bytes_transferred) {
    //TODO: handle error messages properly
    if (error != 0) {
        printf("gbd-remote: Recieved error code %d\n", error);
        isRunning = false;
    }
    bufferMutex->Lock();
    buf_size += bytes_transferred;
    bufferMutex->Unlock();
}
void gdb_remote::async_fillBuffer() {
    if (buf_size == 0) {
        buf_pos = 0;
        buf_size = 0;
    }
    socket->async_receive(buffer(buf+buf_size, BUFFER_SIZE-buf_size), async_fillBufferCallback(this));
}

unsigned long long gdb_remote::readHex(int numDigits) {
    unsigned long long result = 0;
    for (int i = 0; i < numDigits; i++) {
        int c = readChar();
        if (c == ' ') {
            if (i == 0) {
                i--;
                continue;
            } else
                break;
        }
        if (c == EOF)
            return -1;
        if ((c >= '0') && (c <= '9'))
            c = c - '0';
        else if ((c >= 'a') && (c <= 'f'))
            c = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F'))
            c = c - 'A' + 10;
        else
            return -1;
            
        result <<= 4;
        result |= c;
    }
    return result;
}
unsigned long long gdb_remote::readHex(char* buffer, int numDigits) {
    unsigned long long result = 0;
    for (int i = 0; i < numDigits; i++) {
        int c = buffer[i];
        if (c == ' ') {
            if (i == 0) {
                i--; buffer++;
                continue;
            } else
                break;
        }
        if (c == EOF)
            return -1;
        if ((c >= '0') && (c <= '9'))
            c = c - '0';
        else if ((c >= 'a') && (c <= 'f'))
            c = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F'))
            c = c - 'A' + 10;
        else
            break;
            
        result <<= 4;
        result |= c;
    }
    return result;
}
void gdb_remote::writeHex(unsigned long long num, int numDigits) {
    char hexStr[1024];
    for (int i = 0; i < numDigits; i++) {
        int c = num & 0xF;
        num >>= 4;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        hexStr[1024-1-i] = c;
    }
    putData(hexStr+1024-numDigits, numDigits);
}
void gdb_remote::writeHex(char* buffer, unsigned long long num, int numDigits) {
    for (int i = 0; i < numDigits; i++) {
        int c = num & 0xF;
        num >>= 4;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        buffer[numDigits-1-i] = c;
    }
}

//return actual size of packet
int gdb_remote::getPacket(char* packet, int packetSize) {
    while (true) {
        int c = peekChar();
        if (c != '$')
            return 0;
        buf_pos++; buf_size--;  //move buffer forward as through we read and not peeked
        
        int pos;
        bool failed = false;
        unsigned int checksum = 0;
        for (pos = 0; ; pos++) {
            c = readChar();
            if (c == EOF)
                return 0;
            if (c == '$') { //dropped packet?
                pos = 0;
                checksum = 0;
                continue;
            }
            if (c == '#')   //fin
                break;
            if (pos >= packetSize) {
                pos = 0;
                failed = true;
            }
            
            checksum += c;
            
            if (c == '}') {   //Escaped character
                c = readChar();
                if (c == EOF)
                    return 0;
                if (c == '#')
                    break;
                c ^= 0x20;
            }
            
            if (peekChar() == '*') {    //Run-length encoded sequence
                buf_pos++; buf_size--;
                int count = readChar();
                if (count == EOF)
                    return 0;
                if (count == '#')
                    break;
                    
                count -= 29;
                for (int i = 0; i < count; i++) {
                    if (pos >= packetSize) {
                        pos = 0;
                        failed = true;
                    }
                    packet[pos++] = c;
                }
                continue;
            }
            
            packet[pos] = c;
        }
        int size = pos;
        
        if (!NoAckMode) {
            checksum &= 0xFF;
            
            if (checksum == readHex(2)) {
                putChar('+');
            } else {
                #ifdef DEBUG_GDB_PROTOCOL
                cerr << "gdb-remote: Bad checksum received, requesting resend" << endl;
                #endif
                putChar('-');   //Packet failed to transmit, request resend
                continue;   //Try to read the next packet
            }
        } else
            readHex(2); //Still got to read the characters off the connection
        
        if (failed) {
            cerr << "gdb-remote: packet dropped due to insufficent packet buffer size";
            return 0;
        }
        return size;
    }
}
inline void gdb_remote::sendEmptyPacket() {
    #ifdef DEBUG_GDB_PROTOCOL
    cerr << "gdb-remote: Sent empty packet" << endl;
    #endif
    putData("$#00", 4);
    if (!NoAckMode) {
        char* packet = new char[4];
        packet[0] = '$';
        packet[1] = '#';
        packet[2] = '0';
        packet[3] = '0';
        sendQueue.push(new pair<char*, int>(packet, 4));
    }
}
void gdb_remote::sendPacket(char* data, int size, unsigned int packet_size) {
    char* packet = new char[packet_size];
    unsigned int checksum = 0;
    
    packet[0] = '$';
    int buf_pos = 1;
    
    checksum = 0;
    for (int i = 0; i < size; i++) {
        if ((data[i] == '}') || (data[i] == '#') || (data[i] == '$') || (data[i] == '*')) {
            packet[buf_pos++] = '{';
            packet[buf_pos++] = data[i] ^ 0x20;
            checksum += '{' + (data[i] ^ 0x20);
            continue;
        }
        packet[buf_pos++] = data[i];
        checksum += data[i];
    }
    packet[buf_pos++] = '#';
    checksum &= 0xFF;
    writeHex(packet + buf_pos, checksum, 2);
    buf_pos += 2;
    putData(packet, buf_pos);
    
    #ifdef DEBUG_GDB_PROTOCOL
    char* tmp = new char[size+10];
    memcpy(tmp, data, size);
    tmp[size] = 0;
    cerr << "gdb-remote: Sent packet \"" << tmp << "\"" << endl;
    delete[] tmp;
    #endif
    
    if (!NoAckMode) {
        //put on queue to wait for confirmation
        sendQueue.push(new pair<char*, int>(packet, buf_pos));
    } else
        delete[] packet;
}

gdb_remote::gdb_remote(cpu* target, unsigned int port): wxThread(wxTHREAD_JOINABLE), isRunning(false),
    socket(NULL), NoAckMode(false), buf_pos(0), buf_size(0), target(target), port(port), breakpoint_hit(0) {
    
    breakpointMutex = new wxMutex();
    bufferMutex = new wxMutex();
    target->attachDebugger(this);
}
gdb_remote::~gdb_remote() {
    breakpointMutex->Lock();
    delete breakpointMutex;
    bufferMutex->Lock();
    delete bufferMutex;
}
//Called when there is buffer input to handle
void gdb_remote::handleBuffer() {
    int c = peekChar();
    if (c == '-') {
        #ifdef DEBUG_GDB_PROTOCOL
        cerr << "gdb-remote: Resending bad packet" << endl;
        #endif
        if (!sendQueue.empty()) {
            pair<char*, int>* item = sendQueue.front();
            putData(item->first, item->second);
            sendQueue.push(item);
            sendQueue.pop();
        }
        buf_pos++; buf_size--;
        return;
    } else if (c == '+') {
        if (!sendQueue.empty()) {
            #ifdef DEBUG_GDB_PROTOCOL
            cerr << "gdb-remote: Packet successfully sent" << endl;
            #endif
            pair<char*, int>* item = sendQueue.front();
            delete[] item->first;
            delete item;
            sendQueue.pop();
        }
        buf_pos++; buf_size--;
        return;
        
    } else if (c == '$') {
        char packet[BUFFER_SIZE];
        int packetSize = getPacket(packet, BUFFER_SIZE);
        if (packetSize == 0) {
            if (peekChar() == EOF) {
                isRunning = true;
                return;
            }
            return;
        }
        
        packet[packetSize] = 0;
        #ifdef DEBUG_GDB_PROTOCOL
        std::cout << "gdb-remote: Packet \"" << packet << "\" received" << endl;
        #endif
        if (strncmp(packet, "qSupported", 10) == 0) {
            //TODO: parse gdb reported features as needed
            char* response = "QStartNoAckMode+;qSupported:PacketSize=00000400;qXfer:features:read+";
            //char* tmp = strchr(response, 'F');
            //if (BUFFER_SIZE <= 0xFFFFFFFF)
            //    writeHex(tmp, BUFFER_SIZE, 8);
            sendPacket(response, strlen(response));
        } else if (strcmp(packet, "QStartNoAckMode") == 0) {
            sendPacket("OK", 2);
            NoAckMode = true;
            
        } else if (strncmp(packet, "qXfer:features:read", 19) == 0) {
            char* nextOpt = strchr(packet+18, ':');
            char* nextOpt2 = strchr(nextOpt+1, ':');
            if (strncmp(nextOpt+1, "target.xml", nextOpt2-(nextOpt+1)) == 0) {
                nextOpt = nextOpt2;
                nextOpt2 = strchr(nextOpt+1, ',');
                unsigned long long offset = readHex(nextOpt+1, nextOpt2-(nextOpt+1));
                unsigned long long length = readHex(nextOpt2+1, 100000);
                
                char buffer[2049] = {'m'};
                FILE* targetxml = fopen("dcpu16.gdb.xml", "r");
                if (targetxml == NULL) {
                    buffer[0] = 'E';
                    writeHex(buffer+1, errno, 2);
                    sendPacket(buffer, 3);
                    return;
                }
                for (int i = 0; i < offset; i++)
                    fgetc(targetxml);
                //fseek(targetxml, offset, SEEK_SET);
                int i;
                for (i = 0; (i < 1024) && (i < length); i++) {
                    int c = fgetc(targetxml);
                    if (c == EOF) {
                        buffer[0] = 'l';
                        break;
                    }
                    //writeHex(buffer+1+i*2, c, 2);
                    if (c == '\t' || c == '\n' || c == '\r')
                        c = ' ';
                    buffer[1+i] = c;
                }
                fclose(targetxml);
                sendPacket(buffer, 1+i);
            } else
                sendEmptyPacket();
            
        } else if (strncmp(packet, "g", 1) == 0) {
            int count = target->getNumRegisters();
            int vsize = target->getRegisterSize()*2;
            char* packetI = packet;
            for (int i = 0; i < count; i++) {
                unsigned long long tmp = target->getRegister(i);
                /* if (i == cpu::REG_PC || i == cpu::REG_SP) {
                    writeHex(packetI, tmp << 1, 4*2);   //gdb protocol currently takes a byte-based address that uses 32 bits
                    packetI += 4*2;
                } else { */
                    writeHex(packetI, tmp, vsize);
                    packetI += vsize;
                //}
            }
            sendPacket(packet, packetI-packet);
            
        } else if (strncmp(packet, "G", 1) == 0) {
            unsigned int count = target->getNumRegisters();
            unsigned int vsize = target->getRegisterSize()*2;
            char* packetI = packet + 1;
            if (packetSize >= count*vsize) {
                for (int i = 0; i < count; i++) {
                    unsigned long long tmp;
                    if (i == cpu::REG_PC || i == cpu::REG_SP) {
                        tmp = (readHex(packetI, 4*2) >> 0) & 0xFFFF;
                        packetI += 4*2;
                    } else {
                        tmp = readHex(packetI, vsize);
                        packetI += vsize;
                    }
                        
                    target->setRegister(i, tmp);
                }
                sendPacket("OK", 2);
            } else 
                sendPacket("E00", 3);
            
        } else if (strncmp(packet, "m", 1) == 0) {
            unsigned long long rsize = target->getRamSize();
            unsigned int vsize = target->getRamValueSize()*2;
            
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));// / (vsize/2);
            if (nextOpt == NULL) {
                sendPacket("E00", 3);
                return;
            }
            unsigned long long len = readHex(nextOpt+1, 10000);// / (vsize/2); //TODO: handle cases with odd number of bytes?
            if (offset > rsize-1) {
                sendPacket("E00", 3);
                return;
            }
            if (offset+len > rsize-1) {
                len -= (offset + len) - rsize-1;
            }
            
            char* data = new char[len*vsize+10];
            for (int i = 0; i < len; i++) {
                unsigned long long pos = offset + i;
                unsigned long long tmp = target->getRam(pos);
                writeHex(data+i*vsize, tmp, vsize);
            }
            sendPacket(data, len*vsize);
            delete[] data;
            
        } else if (strncmp(packet, "M", 1) == 0) {
            unsigned long long rsize = target->getRamSize();
            unsigned int vsize = target->getRamValueSize()*2;
            
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));// / (vsize/2);
            if (nextOpt == NULL) {
                sendPacket("E00", 3);
                return;
            }
            unsigned long long len = readHex(nextOpt+1, 10000);// / (vsize/2); //TODO: handle cases with odd number of bytes?
            
            nextOpt = strchr(nextOpt+1, ':');
            if (nextOpt == NULL) {
                sendPacket("E00", 3);
                return;
            }
            nextOpt += 1;
            
            if (offset > rsize-1) {
                sendPacket("E00", 3);
                return;
            }
            if (offset+len > rsize-1) {
                len -= (offset + len) - rsize-1;
            }
            
            for (int i = 0; i < len; i++) {
                unsigned long long pos = offset + i;
                unsigned long long val;
                val = readHex(nextOpt+i*vsize, vsize);
                target->setRam(pos, val);
            }
            sendPacket("OK", 2);
            
        } else if (strcmp(packet, "?") == 0) {
            sendPacket("S05", 3);   //report reason execution stopped
            
        } else if (strncmp(packet, "c", 1) == 0) {
            if (packetSize > 1) {
                unsigned long long address = readHex(packet+1, 10000);
                //TODO: continue from address
                sendEmptyPacket();
            } else {
                target->debug_run();
            }
            
        } else if (strncmp(packet, "s", 1) == 0) {
            target->debug_step();
            //cpu will use the onCpuStop() callback to inform us when the step is complete
            
        } else if (strncmp(packet, "Z1", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            if (target->debug_setBreakpoint(address))
                sendPacket("OK", 2);
            else
                sendEmptyPacket();
                
        } else if (strncmp(packet, "z1", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            if (target->debug_clearBreakpoint(address))
                sendPacket("OK", 2);
            else
                sendEmptyPacket();
            
        } else if (strncmp(packet, "Z2", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_setWatchpoint_w(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
                
        } else if (strncmp(packet, "z2", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_clearWatchpoint_w(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
            
        } else if (strncmp(packet, "Z3", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_setWatchpoint_r(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
                
        } else if (strncmp(packet, "z3", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_clearWatchpoint_r(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
            
        } else if (strncmp(packet, "Z4", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_setWatchpoint_a(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
                
        } else if (strncmp(packet, "z4", 2) == 0) {
            char* nextOpt = strchr(packet+1, ',');
            unsigned long long address = readHex(nextOpt+1, 10000);
            nextOpt = strchr(nextOpt+1, ',');
            unsigned long long kind = readHex(nextOpt+1, 10000);
            bool OK = true;
            for (int i = 0; i < kind; i++) {
                if (! target->debug_clearWatchpoint_a(address+i)) {
                    sendEmptyPacket();
                    OK = false;
                    break;
                }
            }
            if (OK)
                sendPacket("OK", 2);
                
        } else
            sendEmptyPacket();  //Unsupported cmd
            
    } else {
        buf_pos++; buf_size--;  //ignore unrecognized behavior
    }
}
wxThread::ExitCode gdb_remote::Entry() {
    io_service io;
    tcp::acceptor acceptor(io, tcp::endpoint(address_v4::loopback(), port));
    
    socket = new tcp::socket(io);
    acceptor.accept(*socket);    //wait for connection from gdb
    
    async_fillBuffer(); //Request first fill
    while (isRunning) {
        breakpointMutex->Lock();
        while (breakpoint_hit > 0) {
            sendPacket("S05", 3);
            breakpoint_hit--;
        }
        breakpointMutex->Unlock();
        
        io.poll();
        io.reset();
        
        bufferMutex->Lock();
        if (buf_size != 0) {
            handleBuffer();
            //If the buffer is empty, request for the buffer to be filled
            //  in the background in case if something comes in
            if (buf_size == 0) {
                async_fillBuffer();
            }
        } else
            Sleep(100);
        bufferMutex->Unlock();
    }
    
    socket->shutdown(tcp::socket::shutdown_both);
    delete socket;
    buf_pos = 0;
    buf_size = 0;
    return 0;
}
void gdb_remote::onCpuStop() {
    breakpointMutex->Lock();
    breakpoint_hit++;
    breakpointMutex->Unlock();
}
