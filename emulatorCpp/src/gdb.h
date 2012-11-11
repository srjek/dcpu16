#include <queue>
#include <boost/asio.hpp>
#include <wx/wx.h>

#include "thread.h"

#ifndef emulator_gdb_h
#define emulator_gdb_h

#define DEBUG_GDB_PROTOCOL

class gdb_remote;
#include "cpu.h"

class gdb_remote: public thread, public wxThread {
public:
    volatile bool isRunning;
protected:
    boost::asio::ip::tcp::socket* socket;
    cpu* target;
    unsigned int port;
    
    //input buffer
    static const int BUFFER_SIZE = 1024*16;
    unsigned char buf[BUFFER_SIZE];
    unsigned int buf_pos;
    unsigned int buf_size;
    
    //messages waiting for confirmation
    std::queue<std::pair<char*, int>*> sendQueue;
    
    bool NoAckMode;
    
    unsigned int breakpoint_hit;
    wxMutex* breakpointMutex;
    
    int peekChar();
    int readChar();
    void putChar(char c);
    void putData(char* c, int size);
    
    unsigned long long readHex(int numDigits);
    unsigned long long readHex(char* buffer, int numDigits);
    void writeHex(unsigned long long num, int numDigits);
    void writeHex(char* buffer, unsigned long long num, int numDigits);

    //return actual size of packet
    int getPacket(char* packet, int packetSize);
    void sendEmptyPacket();
    void sendPacket(char* data, int size, unsigned int packet_size = BUFFER_SIZE);
    
public:
    gdb_remote(cpu* target, unsigned int port);
    ~gdb_remote();
    wxThread::ExitCode Entry();

    void breakpointHit();

    inline wxThreadError Create() {
        return wxThread::Create(10*1024*1024); //10 MB
    }
    inline wxThreadError Run() {
        if (isRunning)
            return wxTHREAD_RUNNING; 
        isRunning = true;
        return wxThread::Run();
    }
    inline void Stop() {
        isRunning = false;
    }
    inline wxThread::ExitCode Wait() {
        if (IsRunning())
            return wxThread::Wait();
        return 0;
    }
};

#endif
