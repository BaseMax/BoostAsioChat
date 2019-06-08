#include <iostream>
#include <thread>
#include <cstdlib>
#include <deque>
#include <boost/asio.hpp>
#include "message.hpp"
using boost::asio::ip::tcp;
using namespace std;
typedef deque<message> messageQueue;
class client {
    public:
        client(boost::asio::io_context& context, const tcp::resolver::results_type& endpoints) : context(context), socket(context) {
            connect(endpoints);
        }
        void write(const message& messageItem) {
            boost::asio::post(context, [this, messageItem]() {
                bool write_in_progress = !writeMessage.empty();
                writeMessage.push_back(messageItem);
                if(!write_in_progress) {
                    write();
                }
            });
        }
        void close() {
            boost::asio::post(context, [this]() { socket.close(); });
        }
    private:
        void connect(const tcp::resolver::results_type& endpoints) {
            boost::asio::async_connect(socket, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
                if(!ec) {
                    readHeader();
                }
            });
        }
        void readHeader() {
            boost::asio::async_read(socket, boost::asio::buffer(readMessage.data(), message::header_length), [this](boost::system::error_code ec, size_t) {
                if(!ec && readMessage.decodeHeader()) {
                    readBody();
                }
                else {
                    socket.close();
                }
            });
        }
        void readBody() {
            boost::asio::async_read(socket, boost::asio::buffer(readMessage.body(), readMessage.bodyLength()), [this](boost::system::error_code ec, size_t) {
                if(!ec) {
                    cout.write(readMessage.body(), readMessage.bodyLength());
                    cout << "\n";
                    readHeader();
                }
                else {
                    socket.close();
                }
            });
        }
        void write() {
            boost::asio::async_write(socket, boost::asio::buffer(writeMessage.front().data(), writeMessage.front().length()), [this](boost::system::error_code ec, size_t) {
                if(!ec) {
                    writeMessage.pop_front();
                    if(!writeMessage.empty()) {
                        write();
                    }
                }
                else {
                    socket.close();
                }
            });
        }
        boost::asio::io_context& context;
        tcp::socket socket;
        message readMessage;
        messageQueue writeMessage;
};
int main(int argc, char* argv[]) {
    try {
        if(argc != 3) {
            cerr << "Usage: client <host> <port>\n";
            return 1;
        }
        boost::asio::io_context context;
        tcp::resolver resolver(context);
        auto endpoints = resolver.resolve(argv[1], argv[2]);
        client c(context, endpoints);
        thread t([&context](){ context.run(); });
        char line[message::max_bodyLength + 1];
        while(cin.getline(line, message::max_bodyLength + 1)) {
            message messageItem;
            messageItem.bodyLength(strlen(line));
            memcpy(messageItem.body(), line, messageItem.bodyLength());
            messageItem.encodeHeader();
            c.write(messageItem);
        }
        c.close();
        t.join();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
