#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>
#include "../src/gdb.cpp"

int main() {
    try {
        boost::asio::io_service io_service;
        gdb_remote* test = new gdb_remote(io_service);
        test->Entry();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
