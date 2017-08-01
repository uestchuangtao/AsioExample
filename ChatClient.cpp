//
// Created by ht on 17-7-11.
//


#include "ChatMessage.h"

#include <deque>
#include <iostream>
#include <exception>
#include <thread>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

using std::deque;
using boost::asio::ip::tcp;


typedef deque<ChatMessage> ChatMessageQueue;

class ChatClient : boost::noncopyable {
public:
    ChatClient(boost::asio::io_service &io_service, tcp::resolver::iterator endpoint_iterator)
            : io_service_(io_service),
              socket_(io_service_)
    {
        tcp::endpoint endpoint = *endpoint_iterator;
        socket_.async_connect(endpoint, boost::bind(&ChatClient::handle_connect, this, boost::asio::placeholders::error,
                                                    ++endpoint_iterator));
    }

    void write(const ChatMessage &msg)
    {
        io_service_.post(boost::bind(&ChatClient::do_write, this, msg));
    }

    void close()
    {
        io_service_.post(boost::bind(&ChatClient::do_close, this));
    }

private:
    void handle_connect(const boost::system::error_code &error, tcp::resolver::iterator endpoint_iterator)
    {
        if (!error) {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), ChatMessage::kHeaderLen),
                                    boost::bind(&ChatClient::handle_read_header, this,
                                                boost::asio::placeholders::error));
        }
        else if (endpoint_iterator != tcp::resolver::iterator()) {
            socket_.close();
            tcp::endpoint endpoint = *endpoint_iterator;
            socket_.async_connect(endpoint,
                                  boost::bind(&ChatClient::handle_connect, this, boost::asio::placeholders::error,
                                              ++endpoint_iterator));
        }
    }

    void handle_read_header(const boost::system::error_code &error)
    {
        std::cout << "ChatClient::handle_read_header" << std::endl;
        if (!error && read_msg_.decode_header()) {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                                    boost::bind(&ChatClient::handle_read_body, this, boost::asio::placeholders::error));
        }
        else {
            do_close();
        }
    }

    void handle_read_body(const boost::system::error_code &error)
    {
        std::cout << "ChatClient::handle_read_body" << std::endl;
        if (!error) {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << std::endl;

            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), ChatMessage::kHeaderLen),
                                    boost::bind(&ChatClient::handle_read_header, this,
                                                boost::asio::placeholders::error));

        }
        else {
            do_close();
        }
    }

    void do_write(const ChatMessage &msg)
    {
        std::cout << "ChatClient::do_write" << std::endl;
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().body_length()),
                                     boost::bind(&ChatClient::handle_write, this, boost::asio::placeholders::error));
        }
    }

    void handle_write(const boost::system::error_code &error)
    {
        if (!error) {
            std::cout << "ChatClient::handle_write" << std::endl;
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
                boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(),
                                                                      write_msgs_.front().body_length()),
                                         boost::bind(&ChatClient::handle_write, this,
                                                     boost::asio::placeholders::error));
            }
        }
        else {
            do_close();
        }
    }

    void do_close()
    {
        socket_.close();
    }


private:
    boost::asio::io_service &io_service_;
    tcp::socket socket_;
    ChatMessage read_msg_;
    ChatMessageQueue write_msgs_;
};


int main(int argc, char *argv[])
{
    try {
        if (argc != 3) {
            std::cerr << " Usage: ChatClient <host> <port>\n" << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], argv[2]);
        tcp::resolver::iterator iterator = resolver.resolve(query);

        ChatClient client(io_service, iterator);
        std::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

        char line[ChatMessage::kMaxBodyLen + 1];
        while (std::cin.getline(line, ChatMessage::kMaxBodyLen + 1)) {
            using namespace std;
            ChatMessage msg;
            msg.body_length(strlen(line));
            memcpy(msg.body(), line, msg.body_length());
            msg.encode_header();
            client.write(msg);
        }
        client.close();
        t.join();
    }
    catch (std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    return 0;
}