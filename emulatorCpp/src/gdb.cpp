#include <cstdio>
#include <iostream>
#include <vector>
#include <wx/wx.h>
#include <boost/asio.hpp>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

#include "thread.h"

class gdb_remote: public thread, protected wxThread {
protected:
    io_service& io;
    bool isRunning;
    bool connected;
    tcp::socket* socket;
    static const int BUFFER_SIZE = 1024*16;
    unsigned char buf[BUFFER_SIZE];
    unsigned int buf_pos = 0;
    unsigned int buf_size = 0;
    
    bool NoAckMode;
    
    int peekChar() {
        if (buf_size == 0) {
            buf_pos = 0;
            buf_size = socket->receive(buffer(buf, BUFFER_SIZE));
        }
        if (buf_size == 0)
            return EOF;
        
        return buf[0];
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
                
            result << 4;
            result |= numDigits;
        }
        return result;
    }

    //return actual size of packet
    int getPacket(tcp::socket* socket, char* packet, int packetSize) {
        while (true) {
            int c = peekChar();
            if (c != '$')
                return 0;
            buf_pos++; buf_size--;  //move buffer forward as through we read and not peeked
            
            int pos;
            bool failed = true;
            for (pos = 0; ; pos++) {
                c = readChar();
                if (c == EOF)
                    return 0;
                if (c == '$') { //dropped packet?
                    pos = 0;
                    continue;
                }
                if (c == '#')   //fin
                    break;
                if (pos >= packetSize) {
                    pos = 0;
                    failed = true;
                }
                
                if (c == '{') {   //Escaped character
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
                unsigned int checksum = 0;
                for (int i = 0; i < size; i++)
                    checksum += packet[i];
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
    
public:
    gdb_remote(io_service& io): io(io), isRunning(false), connected(false), socket(NULL), NoAckMode(false) {
    }
    wxThread::ExitCode Entry() {
        tcp::acceptor acceptor(io, tcp::endpoint(address_v4::loopback(), 6498));
        
        socket = new tcp::socket(io);
        acceptor.accept(*socket);    //wait for connection from gdb
        
        
        socket->shutdown(tcp::socket::shutdown_both);
        delete socket;
        buf_pos = 0;
        buf_size = 0;
        return 0;
    }

    wxThreadError Create() {
        return wxThread::Create(10*1024*1024); //10 MB
    }
    wxThreadError Run() {
        if (isRunning)
            return wxTHREAD_RUNNING; 
        isRunning = true;
        return wxThread::Run();
    }
    void Stop() {
        isRunning = false;
    }
    wxThread::ExitCode Wait() {
        if (IsRunning())
            return wxThread::Wait();
        return 0;
    }
};
