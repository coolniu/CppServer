/*!
    \file web_ssl_client_async.cpp
    \brief HTTPS Web asynchronous client example
    \author Ivan Shynkarenka
    \date 14.03.2017
    \copyright MIT License
*/

#include "asio_service.h"

#include "server/asio/web_ssl_client.h"

#include <iostream>
#include <memory>

void Show(const std::shared_ptr<restbed::Response>& response)
{
    std::cout << "*** Response header ***" << std::endl;
    std::cout << "Status Code:    " << response->get_status_code() << std::endl;
    std::cout << "Status Message: " << response->get_status_message().data() << std::endl;
    std::cout << "HTTP Version:   " << response->get_version() << std::endl;
    std::cout << "HTTP Protocol:  " << response->get_protocol().data() << std::endl;
    for (auto& header : response->get_headers())
        std::cout << "Header ['" << header.first.data() << "'] = '" << header.second.data() << "'" << std::endl;

    std::cout << "*** Response body ***" << std::endl;
    auto length = response->get_header("Content-Length", 0);
    auto content = CppServer::Asio::WebClient::Fetch(response, length);
    std::cout.write((char*)content.data(), content.size());
    std::cout << std::endl << "*** Response end ***" << std::endl;
}

int main(int argc, char** argv)
{
    // HTTPS Web server address
    std::string address = "https://www.google.com";
    if (argc > 1)
        address = argv[1];

    std::cout << "HTTPS Web server address: " << address << std::endl;

    // Create a new Asio service
    auto service = std::make_shared<AsioService>();

    // Start the service
    std::cout << "Asio service starting...";
    service->Start();
    std::cout << "Done!" << std::endl;

    // Create a new HTTPS Web client
    auto client = std::make_shared<CppServer::Asio::WebClient>(service);

    // Create and fill Web request
    auto request = std::make_shared<restbed::Request>(restbed::Uri(address));
    request->set_header("Accept", "*/*");

    // Send synchronous Web request to the server
    auto response = client->SendAsync(request, [](const std::shared_ptr<restbed::Request>& request, const std::shared_ptr<restbed::Response>& response)
    {
        // Show the Web response
        Show(response);
    });

    std::cout << "Press Enter to stop the client..." << std::endl;
    std::string line;
    getline(std::cin, line);

    try
    {
        if (response.valid())
            response.get();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
    }

    // Stop the service
    std::cout << "Asio service stopping...";
    service->Stop();
    std::cout << "Done!" << std::endl;

    return 0;
}
