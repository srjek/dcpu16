#include <cstdio>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try
    {
        if (argc != 2) {
          std::cerr << "Usage: client <host>" << std::endl;
          return 1;
        }

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], "6498", tcp::resolver::query::address_configured | tcp::resolver::query::numeric_service);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        socket.non_blocking(true);
        
        for (;;) {
            char line[1024];
            fgets(line, 1024, stdin);
            int size = strlen(line);
            size -= 1;
            
            if (size > 4)
                socket.send(boost::asio::buffer(line, size));

            boost::array<char, 128> buf;
            boost::system::error_code error;

            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            
            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            if (error == boost::asio::error::would_block)
                continue;
            else if (error)
                throw boost::system::system_error(error); // Some other error.

            std::cout.write(buf.data(), len);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
