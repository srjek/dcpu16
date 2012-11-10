#include <cstdio>
#include <iostream>
#include <vector>
#include <queue>
#include <wx/wx.h>
#include <boost/asio.hpp>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

#include "thread.h"

class gdb_remote: public thread { //, protected wxThread {
protected:
    io_service& io;
    bool isRunning;
    bool connected;
    tcp::socket* socket;
    
    //input buffer
    static const int BUFFER_SIZE = 1024*16;
    unsigned char buf[BUFFER_SIZE];
    unsigned int buf_pos;
    unsigned int buf_size;
    
    //messages waiting for confirmation
    queue<pair<char*, int>*> sendQueue;
    
    bool NoAckMode;
    
    int peekChar() {
        if (buf_size == 0) {
            buf_pos = 0;
            buf_size = socket->receive(buffer(buf, BUFFER_SIZE));
        }
        if (buf_size == 0)
            return EOF;
        
        return buf[buf_pos];
    }
    int readChar() {
        if (peekChar() == EOF)
            return EOF;
        buf_size--;
        return buf[buf_pos++];
    }
    void putChar(char c) {
        socket->send(buffer(&c, 1));
    }
    void putData(char* c, int size) {
        socket->send(buffer(c, size));
    }
    
    unsigned long long readHex(int numDigits) {
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
    unsigned long long readHex(char* buffer, int numDigits) {
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
    void writeHex(unsigned long long num, int numDigits) {
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
    void writeHex(char* buffer, unsigned long long num, int numDigits) {
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
    int getPacket(tcp::socket* socket, char* packet, int packetSize) {
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
    void sendEmptyPacket() {
        putData("$#00", 4);
    }
    void sendPacket(char* data, int size, unsigned int packet_size = BUFFER_SIZE) {
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
    
public:
    gdb_remote(io_service& io): /*wxThread(wxTHREAD_JOINABLE), */io(io), isRunning(false),
        connected(false), socket(NULL), NoAckMode(false), buf_pos(0), buf_size(0) {
    }
    wxThread::ExitCode Entry() {
        tcp::acceptor acceptor(io, tcp::endpoint(address_v4::loopback(), 6498));
        
        socket = new tcp::socket(io);
        acceptor.accept(*socket);    //wait for connection from gdb
        
        while (true) {
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
                int packetSize = getPacket(socket, packet, BUFFER_SIZE);
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
                    } else {
                        //TODO: continue
                    }
                    
                } else if (strncmp(packet, "g", 1) == 0) {
                    //TODO: get register values
                    unsigned short registers[20];
                    for (int i = 0; i < 20; i++) {
                        registers[i] = i;
                        writeHex(packet + i*4, registers[i], 4);
                    }
                    sendPacket(packet, 20*4);
                    
                } else if (strncmp(packet, "G", 1) == 0) {
                    if (packetSize >= 20*4+1) {
                        unsigned short registers[20];
                        for (int i = 0; i < 20; i++) {
                            registers[i] = readHex(packet + 1 + i*4, 4);
                        }
                        //TODO: set register values
                        cout << "Setting register values to:" << endl;
                        for (int i = 0; i < 20; i++) {
                            cout << "\t" << i << ": " << registers[i] << endl;
                        }
                        sendPacket("OK", 2);
                    } else 
                        sendPacket("E 00", 4);
                    
                } else if (strncmp(packet, "m", 1) == 0) {
                    char* nextOpt = strchr(packet+1, ',');
                    unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));
                    if (nextOpt == NULL) {
                        sendEmptyPacket();
                        continue;
                    }
                    unsigned long long len = readHex(nextOpt+1, 10000) / 2; //TODO: handle cases with odd number of bytes?
                    if (offset > 0xFFF) {
                        sendEmptyPacket();
                        continue;
                    }
                    if (offset+len > 0xFFFF) {
                        len -= (offset + len) - 0xFFFF;
                    }
                    
                    //TODO: get ram values
                    unsigned short ram[0x10000];
                    
                    char* data = new char[len*4+10];
                    for (int i = 0; i < len; i++) {
                        unsigned long long pos = offset + i;
                        ram[pos] = pos;
                        writeHex(data+i*4, ram[pos], 4);
                    }
                    sendPacket(data, len*4);
                    delete[] data;
                    
                } else if (strncmp(packet, "M", 1) == 0) {
                    char* nextOpt = strchr(packet+1, ',');
                    unsigned long long offset = readHex(packet+1, nextOpt-(packet+1));
                    if (nextOpt == NULL) {
                        sendPacket("E 00", 4);
                        continue;
                    }
                    unsigned long long len = readHex(nextOpt+1, 10000) / 2; //TODO: handle cases with odd number of bytes?
                    
                    nextOpt = strchr(nextOpt+1, ':');
                    if (nextOpt == NULL) {
                        sendPacket("E 00", 4);
                        continue;
                    }
                    nextOpt += 1;
                    
                    if (offset > 0xFFF) {
                        sendPacket("E 00", 4);
                        continue;
                    }
                    if (offset+len > 0xFFFF) {
                        len -= (offset + len) - 0xFFFF;
                    }
                    
                    cout << "Setting ram values to:" << endl;
                    for (int i = 0; i < len; i++) {
                        unsigned long long pos = offset + i;
                        unsigned long long val = readHex(nextOpt+i*4, 4);
                        cout << "\t" << pos << ": " << val << endl;
                        //TODO: set ram values
                    }
                    sendPacket("OK", 2);
                    
                } else if (strncmp(packet, "s", 1) == 0) {
                    //TODO: step 1 instruction
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

    wxThreadError Create() {
        return wxTHREAD_NO_ERROR;
        //return wxThread::Create(10*1024*1024); //10 MB
    }
    wxThreadError Run() {
        if (isRunning)
            return wxTHREAD_RUNNING; 
        isRunning = true;
        return wxTHREAD_NO_ERROR;
        //return wxThread::Run();
    }
    void Stop() {
        isRunning = false;
    }
    wxThread::ExitCode Wait() {
        //if (IsRunning())
        //    return wxThread::Wait();
        return 0;
    }
};
