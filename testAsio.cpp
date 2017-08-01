//
// Created by ht on 17-7-31.
//

#include <boost/asio.hpp>
#include <boost/function.hpp>

#include <iostream>


using namespace boost::asio;
using namespace boost::system;
using namespace std;

int main()
{
    io_service io;
    signal_set sig(io, SIGINT, SIGUSR1);

    auto handle1 = [&](const boost::system::error_code &error, int signo) {
        if (error) {
            cout << error.message() << endl;
        }
        else {
            if (signo == SIGINT) {
                cout << "receive:" << signo << endl;
            }
        }
    };

    boost::function<void(const boost::system::error_code &, int)> handle2 = [&](const boost::system::error_code &error,
                                                                                int signo) {
        if (error) {
            cout << error.message() << endl;
        }
        else {
            if (signo != SIGINT) {
                sig.async_wait(handle1);
                sig.async_wait(handle2);
            }
        }
    };

    sig.async_wait(handle1);
    sig.async_wait(handle2);

    io.run();

    std::cout << "exit" << endl;

    return 0;
}

