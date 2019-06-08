#ifndef message_HPP
    #define message_HPP

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>

    class message {
        public:
            enum { header_length = 4 };
            enum { max_bodyLength = 512 };
            message() : bodyLength_(0) {
            }
            const char* data() const {
                return data_;
            }
            char* data() {
                return data_;
            }
            std::size_t length() const {
                return header_length + bodyLength_;
            }
            const char* body() const {
                return data_ + header_length;
            }
            char* body() {
                return data_ + header_length;
            }
            std::size_t bodyLength() const {
                return bodyLength_;
            }
            void bodyLength(std::size_t new_length) {
                bodyLength_ = new_length;
                if(bodyLength_ > max_bodyLength)
                    bodyLength_ = max_bodyLength;
            }
            bool decodeHeader() {
                char header[header_length + 1] = "";
                std::strncat(header, data_, header_length);
                bodyLength_ = std::atoi(header);
                if(bodyLength_ > max_bodyLength) {
                    bodyLength_ = 0;
                    return false;
                }
                return true;
            }
            void encodeHeader() {
                char header[header_length + 1] = "";
                std::sprintf(header, "%4d", static_cast<int>(bodyLength_));
                std::memcpy(data_, header, header_length);
            }
        private:
            char data_[header_length + max_bodyLength];
            std::size_t bodyLength_;
    };
#endif
