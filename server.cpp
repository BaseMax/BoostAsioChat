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
        virtual void deliver(const message& messageItem) = 0;
};
typedef shared_ptr<participant> participantPointer;
class room {
    public:
        void join(participantPointer participant) {
            participants.insert(participant);
            for(auto messageItem: messageRecents)
                participant->deliver(messageItem);
        }
        void deliver(const message& messageItem) {
            messageRecents.push_back(messageItem);
            while(messageRecents.size() > max)
                messageRecents.pop_front();
            for(auto participant: participants)
                participant->deliver(messageItem);
        }
        void leave(participantPointer participant) {
            participants.erase(participant);
        }
    private:
        messageQueue messageRecents;
        enum { max = 200 };
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
        void deliver(const message& messageItem) {
            bool write_in_progress = !Messages.empty();
            Messages.push_back(messageItem);
            if(!write_in_progress)
            {
            write();
            }
        }
    private:
        void readHeader() {
            auto self(shared_from_this());
            boost::asio::async_read(socket_,
            boost::asio::buffer(messageItem.data(), message::header_length), [this, self](boost::system::error_code ec, size_t) {
                if(!ec && messageItem.decodeHeader()) {
                    readBody();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
        }
        void readBody() {
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer(messageItem.body(), messageItem.bodyLength()), [this, self](boost::system::error_code ec, size_t) {
                if(!ec) {
                    room_.deliver(messageItem);
                    readHeader();
                }
                else {
                    room_.leave(shared_from_this());
                }
            });
        }
        void write() {
            auto self(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(Messages.front().data(), Messages.front().length()), [this, self](boost::system::error_code ec, size_t) {
                if(!ec) {
                    Messages.pop_front();
                    if(!Messages.empty()) {
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
        message messageItem;
        messageQueue Messages;
};
class server {
    public:
        server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor(io_context, endpoint) {
            do_accept();
        }
    private:
        void do_accept() {
            acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
                if(!ec) {
                    make_shared<session>(move(socket), room_)->start();
                }
                do_accept();
            });
        }
        tcp::acceptor acceptor;
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
