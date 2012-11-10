#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>

#define NO_WXMAIN
#include "wx/wx.h"

#include "../src/gdb.cpp"
#include "../src/cpus/dcpu16/dcpu16.cpp"
#include "../src/strHelper.cpp"

/*
int main() {
    try {
        dcpu16 target(false);
        gdb_remote* test = new gdb_remote(&target);
        test->Entry();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
} */

class gdbRemoteTestApp: public wxApp {
public:
    virtual bool OnInit();
    virtual int OnRun();
};
IMPLEMENT_APP(gdbRemoteTestApp)

bool gdbRemoteTestApp::OnInit() {
    if (!wxApp::OnInit())
        return false;
    
    return true;
}
int gdbRemoteTestApp::OnRun() {
    try {
        dcpu16 target(false);
        gdb_remote* test = new gdb_remote(&target, 6498);
        test->isRunning = true;
        test->Entry();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return wxApp::OnRun();
} 
