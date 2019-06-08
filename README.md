# BoostAsioChat

Simple Chat Application using Boost Asio, based on Cpp.

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

