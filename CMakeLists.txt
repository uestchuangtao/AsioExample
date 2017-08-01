cmake_minimum_required(VERSION 3.7)
project(AsioExample)

set(CMAKE_CXX_STANDARD 11)

set(CLIENT_SOURCE
        ChatMessage.h
        ChatClient.cpp)

set(SERVER_SOURCE
        ChatMessage.h
        ChatServer.cpp)

set(TEST_SOURCE
        testAsio.cpp)

set(BOOST_ROOT "/usr/include/boost")

#add include path
include_directories(/usr/include)

#add lib path
include_directories(/usr/lib)

add_executable(ChatServer ${SERVER_SOURCE})
target_link_libraries(ChatServer pthread boost_system)

add_executable(ChatClient ${CLIENT_SOURCE})
target_link_libraries(ChatClient pthread boost_system)


add_executable(TestAsio ${TEST_SOURCE})
target_link_libraries(TestAsio boost_system pthread)
