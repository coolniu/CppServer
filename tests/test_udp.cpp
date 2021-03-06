//
// Created by Ivan Shynkarenka on 27.12.2016
//

#include "catch.hpp"

#include "server/asio/udp_client.h"
#include "server/asio/udp_server.h"
#include "threads/thread.h"

#include <atomic>
#include <chrono>
#include <vector>

using namespace CppCommon;
using namespace CppServer::Asio;

class EchoUDPService : public Service
{
public:
    std::atomic<bool> thread_initialize;
    std::atomic<bool> thread_cleanup;
    std::atomic<bool> started;
    std::atomic<bool> stopped;
    std::atomic<bool> idle;
    std::atomic<bool> error;

    explicit EchoUDPService()
        : thread_initialize(false),
          thread_cleanup(false),
          started(false),
          stopped(false),
          idle(false),
          error(false)
    {
    }

protected:
    void onThreadInitialize() override { thread_initialize = true; }
    void onThreadCleanup() override { thread_cleanup = true; }
    void onStarted() override { started = true; }
    void onStopped() override { stopped = true; }
    void onIdle() override { idle = true; }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

class EchoUDPClient : public UDPClient
{
public:
    std::atomic<bool> connected;
    std::atomic<bool> disconnected;
    std::atomic<bool> error;

    explicit EchoUDPClient(std::shared_ptr<EchoUDPService> service, const std::string& address, int port)
        : UDPClient(service, address, port),
          connected(false),
          disconnected(false),
          error(false)
    {
    }

protected:
    void onConnected() override { connected = true; }
    void onDisconnected() override { disconnected = true; }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

class EchoUDPServer : public UDPServer
{
public:
    std::atomic<bool> started;
    std::atomic<bool> stopped;
    std::atomic<bool> error;

    explicit EchoUDPServer(std::shared_ptr<EchoUDPService> service, InternetProtocol protocol, int port)
        : UDPServer(service, protocol, port),
          started(false),
          stopped(false),
          error(false)
    {
    }

protected:
    void onStarted() override { started = true; }
    void onStopped() override { stopped = true; }
    void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) override { Send(endpoint, buffer, size); }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

TEST_CASE("UDP server", "[CppServer][Asio]")
{
    const std::string address = "127.0.0.1";
    const int port = 2222;

    // Create and start Asio service
    auto service = std::make_shared<EchoUDPService>();
    REQUIRE(service->Start());
    while (!service->IsStarted())
        Thread::Yield();

    // Create and start Echo server
    auto server = std::make_shared<EchoUDPServer>(service, InternetProtocol::IPv4, port);
    REQUIRE(server->Start());
    while (!server->IsStarted())
        Thread::Yield();

    // Create and connect Echo client
    auto client = std::make_shared<EchoUDPClient>(service, address, port);
    REQUIRE(client->Connect());
    while (!client->IsConnected())
        Thread::Yield();

    // Send a message to the Echo server
    client->Send("test");

    // Wait for all data processed...
    while (client->bytes_received() != 4)
        Thread::Yield();

    // Disconnect the Echo client
    REQUIRE(client->Disconnect());
    while (client->IsConnected())
        Thread::Yield();

    // Stop the Echo server
    REQUIRE(server->Stop());
    while (server->IsStarted())
        Thread::Yield();

    // Stop the Asio service
    REQUIRE(service->Stop());
    while (service->IsStarted())
        Thread::Yield();

    // Check the Asio service state
    REQUIRE(service->thread_initialize);
    REQUIRE(service->thread_cleanup);
    REQUIRE(service->started);
    REQUIRE(service->stopped);
    REQUIRE(!service->idle);
    REQUIRE(!service->error);

    // Check the Echo server state
    REQUIRE(server->started);
    REQUIRE(server->stopped);
    REQUIRE(server->bytes_sent() == 4);
    REQUIRE(server->bytes_received() == 4);
    REQUIRE(!server->error);

    // Check the Echo client state
    REQUIRE(client->connected);
    REQUIRE(client->disconnected);
    REQUIRE(client->bytes_sent() == 4);
    REQUIRE(client->bytes_received() == 4);
    REQUIRE(!client->error);
}

TEST_CASE("UDP server random test", "[CppServer][Asio]")
{
    const std::string address = "127.0.0.1";
    const int port = 2224;

    // Create and start Asio service
    auto service = std::make_shared<EchoUDPService>();
    REQUIRE(service->Start());
    while (!service->IsStarted())
        Thread::Yield();

    // Create and start Echo server
    auto server = std::make_shared<EchoUDPServer>(service, InternetProtocol::IPv4, port);
    REQUIRE(server->Start());
    while (!server->IsStarted())
        Thread::Yield();

    // Test duration in seconds
    const int duration = 10;

    // Clients collection
    std::vector<std::shared_ptr<EchoUDPClient>> clients;

    // Start random test
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < duration)
    {
        // Create a new client and connect
        if ((rand() % 100) == 0)
        {
            if (clients.size() < 100)
            {
                // Create and connect Echo client
                auto client = std::make_shared<EchoUDPClient>(service, address, port);
                clients.emplace_back(client);
                client->Connect();
                while (!client->IsConnected())
                    Thread::Yield();
            }
        }
        // Connect/Disconnect the random client
        else if ((rand() % 100) == 0)
        {
            if (!clients.empty())
            {
                size_t index = rand() % clients.size();
                auto client = clients.at(index);
                if (client->IsConnected())
                {
                    client->Disconnect();
                    while (client->IsConnected())
                        Thread::Yield();
                }
                else
                {
                    client->Connect();
                    while (!client->IsConnected())
                        Thread::Yield();
                }
            }
        }
        // Reconnect the random client
        else if ((rand() % 100) == 0)
        {
            if (!clients.empty())
            {
                size_t index = rand() % clients.size();
                auto client = clients.at(index);
                if (client->IsConnected())
                {
                    client->Reconnect();
                    while (!client->IsConnected())
                        Thread::Yield();
                }
            }
        }
        // Send a message from the random client
        else if ((rand() % 1) == 0)
        {
            if (!clients.empty())
            {
                size_t index = rand() % clients.size();
                auto client = clients.at(index);
                if (client->IsConnected())
                    client->Send("test");
            }
        }

        // Sleep for a while...
        Thread::Sleep(1);
    }

    // Stop the Echo server
    REQUIRE(server->Stop());
    while (server->IsStarted())
        Thread::Yield();

    // Stop the Asio service
    REQUIRE(service->Stop());
    while (service->IsStarted())
        Thread::Yield();

    // Check the Echo server state
    REQUIRE(server->started);
    REQUIRE(server->stopped);
    REQUIRE(server->bytes_sent() > 0);
    REQUIRE(server->bytes_received() > 0);
    REQUIRE(!server->error);
}
