//
// Created by ht on 17-7-11.
//

#ifndef ASIOEXAMPLE_CHATMESSAGE_H
#define ASIOEXAMPLE_CHATMESSAGE_H

#include <vector>
#include <stdlib.h>  //atoi
#include <string.h>  //memcpy
#include <stdio.h> //snprintf

using std::vector;

class ChatMessage {
public:
    ChatMessage()
            : bodyLen_(0),
              data_(kHeaderLen + bodyLen_)
    {

    }

    const char *data() const
    {
        return &data_[0];
    }

    char *data()
    {
        return &data_[0];
    }

    char *body()
    {
        return &data_[0] + kHeaderLen;
    }

    const char *body() const
    {
        return &data_[0] + kHeaderLen;
    }

    size_t body_length() const
    {
        return bodyLen_;
    }

    void body_length(size_t length)
    {
        if (length > kMaxBodyLen) {
            length = kMaxBodyLen;
            bodyLen_ = length;
        }
        data_.resize(kHeaderLen + bodyLen_);
    }

    bool decode_header()
    {
        char header[kHeaderLen + 1];
        memset(header, 0, sizeof(header));
        memcpy(header, &data_[0], kHeaderLen);
        bodyLen_ = atoi(header);
        if (bodyLen_ > kMaxBodyLen) {
            bodyLen_ = 0;
            return false;
        }
        data_.resize(kHeaderLen + bodyLen_);
        return true;

    }

    void encode_header()
    {
        char header[kHeaderLen + 1];
        memset(header, 0, sizeof(header));
        sprintf(header, "%4d", static_cast<int>(bodyLen_));
        memcpy(&data_[0], header, kHeaderLen);
    }

public:
    static const int kHeaderLen = 4;
    static const int kMaxBodyLen = 1024;

private:
    size_t bodyLen_;
    vector<char> data_;


};

#endif //ASIOEXAMPLE_CHATMESSAGE_H
