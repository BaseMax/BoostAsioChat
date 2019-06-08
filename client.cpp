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
        client(boost::asio::io_context& context, const tcp::resolver::results_type& endpoints) : context_(context), socket_(context) {
            do_connect(endpoints);
        }
        void write(const message& msg) {
            boost::asio::post(context_, [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if(!write_in_progress) {
                    write();
                }
            });
        }
        void close() {
            boost::asio::post(context_, [this]() { socket_.close(); });
        }
    private:
        void do_connect(const tcp::resolver::results_type& endpoints) {
            boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
                if(!ec) {
                    readHeader();
                }
            });
        }
        void readHeader() {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), message::header_length), [this](boost::system::error_code ec, size_t) {
                if(!ec && read_msg_.decodeHeader()) {
                    readBody();
                }
                else {
                    socket_.close();
                }
            });
        }
        void readBody() {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.bodyLength()), [this](boost::system::error_code ec, size_t) {
                if(!ec) {
                    cout.write(read_msg_.body(), read_msg_.bodyLength());
                    cout << "\n";
                    readHeader();
                }
                else {
                    socket_.close();
                }
            });
        }
        void write() {
            boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), [this](boost::system::error_code ec, size_t) {
                if(!ec) {
                    write_msgs_.pop_front();
                    if(!write_msgs_.empty()) {
                        write();
                    }
                }
                else {
                    socket_.close();
                }
            });
        }
        boost::asio::io_context& context_;
        tcp::socket socket_;
        message read_msg_;
        messageQueue write_msgs_;
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
            message msg;
            msg.bodyLength(strlen(line));
            memcpy(msg.body(), line, msg.bodyLength());
            msg.encodeHeader();
            c.write(msg);
        }
        c.close();
        t.join();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
