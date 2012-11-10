#include "gdb.h"

//#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

    
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
    putData("$#00", 4);
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
    
    if (!NoAckMode) {
        //put on queue to wait for confirmation
        sendQueue.push(new pair<char*, int>(packet, buf_pos));
    } else
        delete[] packet;
}

gdb_remote::gdb_remote(cpu* target, unsigned int port): wxThread(wxTHREAD_JOINABLE), isRunning(false),
    socket(NULL), NoAckMode(false), buf_pos(0), buf_size(0), target(target), port(port) {
    target->initDebug();
}
wxThread::ExitCode gdb_remote::Entry() {
    io_service io;
    tcp::acceptor acceptor(io, tcp::endpoint(address_v4::loopback(), port));
    
    socket = new tcp::socket(io);
    acceptor.accept(*socket);    //wait for connection from gdb
    
    while (isRunning) {
        int c = peekChar();
        if (c == '-') {
            if (!sendQueue.empty()) {
                pair<char*, int>* item = sendQueue.front();
                putData(item->first, item->second);
                sendQueue.push(item);
                sendQueue.pop();
            }
            buf_pos++; buf_size--;
            continue;
        } else if (c == '+') {
            if (!sendQueue.empty()) {
                pair<char*, int>* item = sendQueue.front();
                delete[] item->first;
                delete item;
                sendQueue.pop();
            }
            buf_pos++; buf_size--;
            continue;
            
        } else if (c == '$') {
            char packet[BUFFER_SIZE];
            int packetSize = getPacket(packet, BUFFER_SIZE);
            if (packetSize == 0) {
                if (peekChar() == EOF)
                    break;
                continue;
            }
            
            packet[packetSize] = 0;
            if (strcmp(packet, "QStartNoAckMode") == 0) {
                sendPacket("OK", 2);
                NoAckMode = true;
                
            } else if (strcmp(packet, "?") == 0) {
                sendPacket("S05", 2);   //report reason execution stopped
                
            } else if (strncmp(packet, "c", 1) == 0) {
                if (packetSize > 1) {
                    unsigned long long address = readHex(packet+1, 10000);
                    //TODO: continue from address
                    sendEmptyPacket();
                } else {
                    target->debug_run();
                }
                
            } else if (strncmp(packet, "g", 1) == 0) {
                int count = target->getNumRegisters();
                int vsize = target->getRegisterSize()*2;
                for (int i = 0; i < count; i++) {
                    unsigned long long tmp = target->getRegister(i);
                    writeHex(packet + i*vsize, tmp, vsize);
                }
                sendPacket(packet, count*vsize);
                
            } else if (strncmp(packet, "G", 1) == 0) {
                unsigned int count = target->getNumRegisters();
                unsigned int vsize = target->getRegisterSize()*2;
                if (packetSize >= count*vsize) {
                    for (int i = 0; i < count; i++) {
                        unsigned long long tmp;
                        tmp = readHex(packet + 1 + i*vsize, vsize);
                        target->setRegister(i, tmp);
                    }
                    sendPacket("OK", 2);
                } else 
                    sendPacket("E 00", 4);
                
            } else if (strncmp(packet, "m", 1) == 0) {
                unsigned long long rsize = target->getRamSize();
                unsigned int vsize = target->getRamValueSize()*2;
                
                char* nextOpt = strchr(packet+1, ',');
                unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));
                if (nextOpt == NULL) {
                    sendEmptyPacket();
                    continue;
                }
                unsigned long long len = readHex(nextOpt+1, 10000) / (vsize/2); //TODO: handle cases with odd number of bytes?
                if (offset > rsize-1) {
                    sendEmptyPacket();
                    continue;
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
                unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));
                if (nextOpt == NULL) {
                    sendPacket("E 00", 4);
                    continue;
                }
                unsigned long long len = readHex(nextOpt+1, 10000) / (vsize/2); //TODO: handle cases with odd number of bytes?
                
                nextOpt = strchr(nextOpt+1, ':');
                if (nextOpt == NULL) {
                    sendPacket("E 00", 4);
                    continue;
                }
                nextOpt += 1;
                
                if (offset > rsize-1) {
                    sendPacket("E 00", 4);
                    continue;
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
                
            } else if (strncmp(packet, "s", 1) == 0) {
                target->debug_step();
                sendPacket("S05", 3);
                
            } else
                sendEmptyPacket();  //Unsupported cmd
                
        } else {
            buf_pos++; buf_size--;  //ignore unrecognized behavior
        }
    }
    
    socket->shutdown(tcp::socket::shutdown_both);
    delete socket;
    buf_pos = 0;
    buf_size = 0;
    return 0;
}
