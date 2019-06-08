#ifndef message_HPP
    #define message_HPP

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    using namespace std;
    class message {
        public:
            enum { headerLength = 4 };
            enum { maxBodyLength = 512 };
            message() : bodyLength_(0) {
            }
            const char* data() const {
                return data_;
            }
            char* data() {
                return data_;
            }
            size_t length() const {
                return headerLength + bodyLength_;
            }
            const char* body() const {
                return data_ + headerLength;
            }
            char* body() {
                return data_ + headerLength;
            }
            size_t bodyLength() const {
                return bodyLength_;
            }
            void bodyLength(size_t new_length) {
                bodyLength_ = new_length;
                if(bodyLength_ > maxBodyLength)
                    bodyLength_ = maxBodyLength;
            }
            bool decodeHeader() {
                char header[headerLength + 1] = "";
                strncat(header, data_, headerLength);
                bodyLength_ = atoi(header);
                if(bodyLength_ > maxBodyLength) {
                    bodyLength_ = 0;
                    return false;
                }
                return true;
            }
            void encodeHeader() {
                char header[headerLength + 1] = "";
                sprintf(header, "%4d", static_cast<int>(bodyLength_));
                memcpy(data_, header, headerLength);
            }
        private:
            char data_[headerLength + maxBodyLength];
            size_t bodyLength_;
    };

#endif
