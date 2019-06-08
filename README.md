# BoostAsioChat

Simple Chat Application using Boost Asio, based on Cpp.


## Structure

### Server

```cpp
typedef deque<message> messageQueue;

class participant {
    public:
        virtual ~participant() {}
        virtual void deliver(const message& messageItem) = 0;
};

typedef shared_ptr<participant> participantPointer;

class room {
    public:
        void join(participantPointer participant);
        void deliver(const message& messageItem);
        void leave(participantPointer participant);
    private:
        messageQueue messageRecents;
        enum { max = 200 };
        set<participantPointer> participants;
};

class session : public participant, public enable_shared_from_this<session> {
    public:
        session(tcp::socket socket, room& room) : socket_(move(socket)), room_(room);
        void start();
        void deliver(const message& messageItem);
    private:
        void readHeader();
        void readBody();
        void write();
        tcp::socket socket_;
        room& room_;
        message messageItem;
        messageQueue Messages;
};

class server {
    public:
        server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor_(io_context, endpoint);
    private:
        void do_accept();
        tcp::acceptor acceptor_;
        room room_;
};

int main(int argc, char* argv[]);
```

### Client

```cpp
typedef deque<message> messageQueue;

class client {
    public:
        client(boost::asio::io_context& context, const tcp::resolver::results_type& endpoints) : context(context), socket(context);
        void write(const message& messageItem);
        void close();
    private:
        void connect(const tcp::resolver::results_type& endpoints);
        void readHeader();
        void readBody();
        void write();
        boost::asio::io_context& context;
        tcp::socket socket;
        message readMessage;
        messageQueue writeMessage;
};

int main(int argc, char* argv[]);
```
### Compile 

```
$ g++ client.cpp -lpthread -o client
$ g++ Server.cpp -lpthread -o server
```

### Using

1. Start the server
```
./server 4000
```
2. Login as a user

```
./client localhost 4000
first user: you can type message here...
```

3. Login as a new user

```
./client localhost 4000
second user: you can type message here...
```

