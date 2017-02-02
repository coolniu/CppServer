/*!
    \file nanomsg_subscriber_client.cpp
    \brief Nanomsg subscriber client example
    \author Ivan Shynkarenka
    \date 03.02.2017
    \copyright MIT License
*/

#include "server/nanomsg/subscriber_client.h"
#include "threads/thread.h"

#include <iostream>
#include <memory>

class ExampleSubscriberClient : public CppServer::Nanomsg::SubscriberClient
{
public:
    using CppServer::Nanomsg::SubscriberClient::SubscriberClient;

protected:
    void onConnected() override
    {
        std::cout << "Nanomsg subscriber client connected" << std::endl;
    }

    void onDisconnected() override
    {
        std::cout << "Nanomsg subscriber client disconnected" << std::endl;

        // Wait for a while...
        CppCommon::Thread::Sleep(1000);

        // Try to connect again
        Connect();
    }

    void onReceived(CppServer::Nanomsg::Message& msg) override
    {
        std::string message((const char*)msg.buffer(), msg.size());
        std::cout << "Incoming: " << message << std::endl;
    }

    void onError(int error, const std::string& message) override
    {
        std::cout << "Nanomsg subscriber client caught an error with code " << error << "': " << message << std::endl;
    }
};

int main(int argc, char** argv)
{
    // Nanomsg publisher server address
    std::string address = "tcp://localhost:6669";
    if (argc > 1)
        address = argv[1];

    std::cout << "Nanomsg publisher server address: " << address << std::endl;
    std::cout << "Press Enter to stop the client or '!' to reconnect the client..." << std::endl;

    // Create a new Nanomsg subscriber client
    auto client = std::make_shared<ExampleSubscriberClient>(address);

    // Start the client
    client->Connect();

    // Perform text input
    std::string line;
    while (getline(std::cin, line))
    {
        if (line.empty())
            break;

        // Disconnect the client
        if (line == "!")
        {
            std::cout << "Client disconnecting...";
            client->Disconnect();
            continue;
        }
    }

    // Disconnect the client
    client->Disconnect();

    return 0;
}