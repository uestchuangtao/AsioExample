//
// Created by ht on 17-7-11.
//

#include "ChatMessage.h"

#include <deque>
#include <iostream>
#include <set>
#include <list>
#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>


using boost::asio::ip::tcp;

typedef std::deque<ChatMessage> ChatMessageQueue;

class ChatParticipant {
public:
    virtual ~ChatParticipant()
    {
    }

    virtual void deliver(const ChatMessage &msg) = 0;
};

typedef boost::shared_ptr<ChatParticipant> ChatParticipantPtr;

class ChatRoom {
public:
    ChatRoom()
            : recent_msgs_(MAX_RECENT_MSGS)
    {

    }

    void join(ChatParticipantPtr participant)
    {
        participants_.insert(participant);
        std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
                      boost::bind(&ChatParticipant::deliver, participant, _1));
    }

    void leave(ChatParticipantPtr participant)
    {
        participants_.erase(participant);
    }

    void deliver(const ChatMessage &msg)
    {
        recent_msgs_.push_back(msg);
        std::for_each(participants_.begin(), participants_.end(),
                      boost::bind(&ChatParticipant::deliver, _1, boost::ref(msg)));
    }

private:
    std::set<ChatParticipantPtr> participants_;
    enum {
        MAX_RECENT_MSGS = 100
    };
    boost::circular_buffer<ChatMessage> recent_msgs_;
};

class ChatSession : public ChatParticipant,
                    public boost::enable_shared_from_this<ChatSession> {
public:
    ChatSession(boost::asio::io_service &io_service, ChatRoom &room)
            : socket_(io_service),
              room_(room)
    {

    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        room_.join(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), ChatMessage::kHeaderLen),
                                boost::bind(&ChatSession::handle_read_header, this, boost::asio::placeholders::error));
    }

    void deliver(const ChatMessage &msg)
    {
        bool write_in_process = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_process) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().body_length()),
                                     boost::bind(&ChatSession::handle_write, shared_from_this(),
                                                 boost::asio::placeholders::error));
        }
    }

private:
    void handle_read_header(const boost::system::error_code &error)
    {
        std::cout << "ChatSession::handle_read_header" << std::endl;
        if (!error && read_msg_.decode_header()) {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                                    boost::bind(&ChatSession::handle_read_body, shared_from_this(),
                                                boost::asio::placeholders::error));
        }
        else {
            room_.leave(shared_from_this());
        }
    }

    void handle_read_body(const boost::system::error_code &error)
    {
        std::cout << "ChatSession::handle_read_body" << std::endl;
        if (!error) {
            std::cout.write(read_msg_.body(), read_msg_.body_length()) << std::endl;
            room_.deliver(read_msg_);
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), ChatMessage::kHeaderLen),
                                    boost::bind(&ChatSession::handle_read_header, shared_from_this(),
                                                boost::asio::placeholders::error));
        }
        else {
            room_.leave(shared_from_this());
        }
    }

    void handle_write(const boost::system::error_code &error)
    {
        if (!error) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
                boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(),
                                                                      write_msgs_.front().body_length()),
                                         boost::bind(&ChatSession::handle_write, shared_from_this(),
                                                     boost::asio::placeholders::error));
            }
        }
        else {
            room_.leave(shared_from_this());
        }
    }


private:
    tcp::socket socket_;
    ChatRoom &room_;
    ChatMessage read_msg_;
    ChatMessageQueue write_msgs_;
};

typedef boost::shared_ptr<ChatSession> ChatSessionPtr;


class ChatServer : boost::noncopyable {
public:
    ChatServer(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
            : io_service_(io_service),
              acceptor_(io_service_, endpoint)
    {
        ChatSessionPtr new_session(new ChatSession(io_service_, room_));
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&ChatServer::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

private:
    void handle_accept(ChatSessionPtr session, const boost::system::error_code &error)
    {
        if (!error) {
            std::cout << "accepted_one" << std::endl;
            session->start();
            ChatSessionPtr new_session(new ChatSession(io_service_, room_));
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&ChatServer::handle_accept, this, new_session,
                                               boost::asio::placeholders::error));
        }
    }

private:
    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
    ChatRoom room_;
};

typedef boost::shared_ptr<ChatServer> ChatServerPtr;
typedef std::list<ChatServerPtr> ChatSeverList;


int main(int argc, char *argv[])
{
    try {
        if (argc < 2) {
            std::cerr << " Usage: ChatServer <port> [<port> ...]" << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;
        ChatSeverList servers;
        for (int i = 1; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
            ChatServerPtr server(new ChatServer(io_service, endpoint));
            servers.push_back(server);
        }

        io_service.run();

    }
    catch (std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    return 0;
}