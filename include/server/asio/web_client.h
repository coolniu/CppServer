/*!
    \file web_client.h
    \brief HTTP Web client definition
    \author Ivan Shynkarenka
    \date 14.03.2017
    \copyright MIT License
*/

#ifndef CPPSERVER_ASIO_WEB_CLIENT_H
#define CPPSERVER_ASIO_WEB_CLIENT_H

#include "asio.h"
#include "service.h"
#include "web.h"

#include <functional>
#include <future>
#include <vector>

namespace CppServer {
namespace Asio {

//! HTTP Web client
/*!
    HTTP Web client is used to send different kind of Web requests
    such as POST, GET, PUT, DELETE, etc. to any HTTP Web server and
    receive responses in synchronous and asynchronous modes.

    Thread-safe.

    https://github.com/corvusoft/restbed
*/
class WebClient
{
public:
    //! Initialize HTTP Web server with a given Asio service
    /*!
        \param service - Asio service
    */
    explicit WebClient(std::shared_ptr<Service> service);
    WebClient(const WebClient&) = delete;
    WebClient(WebClient&&) = default;
    virtual ~WebClient() = default;

    WebClient& operator=(const WebClient&) = delete;
    WebClient& operator=(WebClient&&) = default;

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _service; }
    //! Get the Restbed settings
    std::shared_ptr<restbed::Settings>& settings() noexcept { return _settings; }

    //! Send Web request to the server in synchronous mode
    /*!
        \param request - Web request
        \return Web response
    */
    const std::shared_ptr<restbed::Response> Send(const std::shared_ptr<restbed::Request>& request);
    //! Send Web request to the server in asynchronous mode
    /*!
        \param request - Web request
        \param callback - Callback function (default is empty callback)
        \return Web response future
    */
    std::future<std::shared_ptr<restbed::Response>> SendAsync(const std::shared_ptr<restbed::Request>& request, const std::function<void (const std::shared_ptr<restbed::Request>&, const std::shared_ptr<restbed::Response>&)>& callback = [](const std::shared_ptr<restbed::Request>&, const std::shared_ptr<restbed::Response>&){});

    //! Is the given Web request opened?
    /*!
        \param request - Web request
        \return 'true' if the given Web request opened, 'false' if the given Web request closed
    */
    static bool IsOpened(const std::shared_ptr<restbed::Request>& request);

    //! Close the given Web request
    /*!
        \param request - Web request
    */
    static void Close(const std::shared_ptr<restbed::Request>& request);

    //! Fetch content from the given Web response by the given size
    /*!
        \param response - Web response
        \param size - Content size in bytes
        \return Bytes buffer
    */
    static std::vector<uint8_t> Fetch(const std::shared_ptr<restbed::Response>& response, const size_t size);
    //! Fetch content from the given Web response by the given delimiter
    /*!
        \param response - Web response
        \param delimiter - Content delimiter
        \return Bytes buffer
    */
    static std::vector<uint8_t> Fetch(const std::shared_ptr<restbed::Response>& response, const std::string& delimiter);

    //! Convert Web request to the bytes buffer
    /*!
        \param request - Web request
        \return Bytes buffer
    */
    static std::vector<uint8_t> Convert(const std::shared_ptr<restbed::Request>& request);
    //! Convert Web response to the bytes buffer
    /*!
        \param response - Web response
        \return Bytes buffer
    */
    static std::vector<uint8_t> Convert(const std::shared_ptr<restbed::Response>& response);

private:
    // Asio service
    std::shared_ptr<Service> _service;
    // Restbed server & settings
    std::shared_ptr<restbed::Settings> _settings;
};

/*! \example web_client_sync.cpp HTTP Web synchronous client example */
/*! \example web_client_async.cpp HTTP Web asynchronous client example */

} // namespace Asio
} // namespace CppServer

#endif // CPPSERVER_ASIO_WEB_CLIENT_H
