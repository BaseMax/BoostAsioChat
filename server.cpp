#include <iostream>
#include <cstdlib>
#include <deque>
#include <memory>
#include <list>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "message.hpp"
using boost::asio::ip::tcp;
using namespace std;
typedef deque<message> messageQueue;
class participant {
    public:
        virtual ~participant() {}
        virtual void deliver(const message& msg) = 0;
};
typedef shared_ptr<participant> participantPointer;
class room {
    public:
        void join(participantPointer participant) {
            participants.insert(participant);
            for(auto msg: messageRecents)
                participant->deliver(msg);
        }
        void deliver(const message& msg) {
            messageRecents.push_back(msg);
            while(messageRecents.size() > max_recent_msgs)
                messageRecents.pop_front();
            for(auto participant: participants)
                participant->deliver(msg);
        }
        void leave(participantPointer participant) {
            participants.erase(participant);
        }
    private:
        messageQueue messageRecents;
        enum { max_recent_msgs = 200 };
        set<participantPointer> participants;
};
class session : public participant, public enable_shared_from_this<session> {
    public:
        session(tcp::socket socket, room& room) : socket_(move(socket)), room_(room) {
        }
        void start() {
            room_.join(shared_from_this());
            readHeader();
        }
        void deliver(const message& msg) {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if(!write_in_progress)
            {
            write();
            }
        }
    private:
        void readHeader() {
            auto self(shared_from_this());
            boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg.data(), message::header_length), [this, self](boost::system::error_code ec, size_t) {
                if(!ec && read_msg.decodeHeader()) {
                    readBody();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
        }
        void readBody() {
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg.body(), read_msg.bodyLength()), [this, self](boost::system::error_code ec, size_t) {
                if(!ec) {
                    room_.deliver(read_msg);
                    readHeader();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
        }
        void write() {
            auto self(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), [this, self](boost::system::error_code ec, size_t) {
                if(!ec) {
                    write_msgs_.pop_front();
                    if(!write_msgs_.empty()) {
                        write();
                    }
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
        }
        tcp::socket socket_;
        room& room_;
        message read_msg;
        messageQueue write_msgs_;
};
class server {
    public:
        server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor_(io_context, endpoint) {
            do_accept();
        }
    private:
        void do_accept() {
            acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
                if(!ec) {
                    make_shared<session>(move(socket), room_)->start();
                }
                do_accept();
            });
        }
        tcp::acceptor acceptor_;
        room room_;
};
int main(int argc, char* argv[]) {
    try {
        if(argc < 2) {
            cerr << "Usage: server <port> [<port> ...]\n";
            return 1;
        }
        boost::asio::io_context io_context;
        list<server> servers;
        for(int i = 1; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
            servers.emplace_back(io_context, endpoint);
        }
        io_context.run();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
