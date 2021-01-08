#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/system/error_code.hpp>
#include <iomanip>
#include <iostream>

using beast = boost::beast;
using net = boost::beast::websocket;

void Log(const std::string &where, boost::system::error_code ec) {
    std::cerr << "[" << std::setw(20) << where << "] " << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "") << std::endl;
}
int main() {
    const std::string url{"echo.websocket.org"};
    const std::string port{"80"};
    const std::string message{"Hello world"};

    // always start with an I/O context object
    boost::asio::io_context ioc{};

    // socket.connect uses the I/O context to communicate with the socket
    // and get a response back.
    // the response is saved in ec.
    boost::system::error_code ec{};

    tcp::resolver resolver{ioc};
    auto endpoint{resolver.resolve("echo.websocket.org", "80", ec)};

    if (ec) {
        Log("resolver.resolve", ec);
        return -1;
    }

    // create an I/O object.
    // Every Boost.Asio object API needs an io_context as the first param.
    tcp::socket socket{ioc};
    socket.connect(*endpoint, ec);
    if (ec) {
        Log("socket.connect", ec);
        return -2;
    }

    // websocket::stream<tcp::socket> ws{ioc};
    // create a websocket using the pre-constructed socket object
    // socket has already connected to endpoint so we just pass it in
    websocket::stream<boost::beast::tcp_stream> ws{std::move(socket)};
    ws.handshake(url, "/", ec);
    if (ec) {
        Log("ws.handshake", ec);
        return -3;
    }
    ws.text(true);
    boost::asio::const_buffer wbuffer{message.c_str(), message.size()};
    ws.write(wbuffer, ec);
    if (ec) {
        Log("ws.write", ec);
        return -4;
    }

    beast::flat_buffer buffer;
    ws.read(buffer, ec);

    if (ec) {
        Log("ws.read", ec);
        return -5;
    }
    ws.close(websocket::close_code::normal);

    std::cout << "ECHO: " << beast::make_printable(buffer.data()) << std::endl;
    Log("returning", ec);
    return 0;
}
