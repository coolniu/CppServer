//
// Created by Ivan Shynkarenka on 15.03.2017
//

#include "catch.hpp"

#include "server/asio/web_client.h"
#include "server/asio/web_server.h"
#include "threads/thread.h"

#include <chrono>
#include <memory>
#include <map>

using namespace CppCommon;
using namespace CppServer::Asio;

class HttpServer : public WebServer
{
public:
    explicit HttpServer(std::shared_ptr<Service> service, int port)
        : WebServer(service, port)
    {
        // Create a resource
        auto resource = std::make_shared<restbed::Resource>();
        resource->set_path("/storage/{key: .*}");
        resource->set_method_handler("POST", RestStoragePost);
        resource->set_method_handler("GET", RestStorageGet);
        resource->set_method_handler("PUT", RestStoragePut);
        resource->set_method_handler("DELETE", RestStorageDelete);

        // Publish the resource
        server()->publish(resource);
    }

private:
    static std::map<std::string, std::string> _storage;

    static void RestStoragePost(const std::shared_ptr<restbed::Session>& session)
    {
        auto request = session->get_request();
        size_t request_content_length = request->get_header("Content-Length", 0);
        session->fetch(request_content_length, [request](const std::shared_ptr<restbed::Session> session, const restbed::Bytes & body)
        {
            std::string key = request->get_path_parameter("key");
            std::string data = std::string((char*)body.data(), body.size());

            _storage[key] = data;

            session->close(restbed::OK);
        });
    }

    static void RestStorageGet(const std::shared_ptr<restbed::Session>& session)
    {
        auto request = session->get_request();
        std::string key = request->get_path_parameter("key");
        std::string data = _storage[key];

        session->close(restbed::OK, data, { { "Content-Length", std::to_string(data.size()) } });
    }

    static void RestStoragePut(const std::shared_ptr<restbed::Session>& session)
    {
        const auto request = session->get_request();
        size_t request_content_length = request->get_header("Content-Length", 0);
        session->fetch(request_content_length, [request](const std::shared_ptr<restbed::Session> session, const restbed::Bytes & body)
        {
            std::string key = request->get_path_parameter("key");
            std::string data = std::string((char*)body.data(), body.size());

            _storage[key] = data;

            session->close(restbed::OK);
        });
    }

    static void RestStorageDelete(const std::shared_ptr<restbed::Session>& session)
    {
        auto request = session->get_request();
        std::string key = request->get_path_parameter("key");
        std::string data = _storage[key];

        _storage[key] = "";

        session->close(restbed::OK);
    }
};

std::map<std::string, std::string> HttpServer::_storage;

TEST_CASE("HTTP Web server", "[CppServer][Asio]")
{
    const std::string address = "127.0.0.1";
    const int port = 8000;
    const std::string uri = "http://" + address + ":" + std::to_string(port) + "/storage/test";

    // Create and start Asio service
    auto service = std::make_shared<Service>();
    REQUIRE(service->Start());
    while (!service->IsStarted())
        Thread::Yield();

    // Create and start HTTP Web server
    auto server = std::make_shared<HttpServer>(service, port);
    REQUIRE(server->Start());
    while (!server->IsStarted())
        Thread::Yield();

    // Create a new HTTP Web client
    auto client = std::make_shared<CppServer::Asio::WebClient>(service);

    // Send a GET request to the HTTP Web server
    auto request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("GET");
    auto response = client->Send(request);
    REQUIRE(response != nullptr);
    auto length = response->get_header("Content-Length", 0);
    WebClient::Fetch(response, length);
    REQUIRE(response->get_body().size() == 0);

    // Send a POST request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("POST");
    request->set_header("Content-Length", "3");
    request->set_body("123");
    response = client->Send(request);
    REQUIRE(response != nullptr);

    // Send a GET request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("GET");
    response = client->Send(request);
    REQUIRE(response != nullptr);
    length = response->get_header("Content-Length", 0);
    WebClient::Fetch(response, length);
    REQUIRE(response->get_body().size() == 3);

    // Send a PUT request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("PUT");
    request->set_header("Content-Length", "6");
    request->set_body("123456");
    response = client->Send(request);
    REQUIRE(response != nullptr);

    // Send a GET request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("GET");
    response = client->Send(request);
    REQUIRE(response != nullptr);
    length = response->get_header("Content-Length", 0);
    WebClient::Fetch(response, length);
    REQUIRE(response->get_body().size() == 6);

    // Send a DELETE request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("DELETE");
    response = client->Send(request);
    REQUIRE(response != nullptr);

    // Send a GET request to the HTTP Web server
    request = std::make_shared<restbed::Request>(restbed::Uri(uri));
    request->set_method("GET");
    response = client->Send(request);
    REQUIRE(response != nullptr);
    length = response->get_header("Content-Length", 0);
    WebClient::Fetch(response, length);
    REQUIRE(response->get_body().size() == 0);

    // Stop the HTTP Web server
    REQUIRE(server->Stop());
    while (server->IsStarted())
        Thread::Yield();

    // Stop the Asio service
    REQUIRE(service->Stop());
    while (service->IsStarted())
        Thread::Yield();
}
