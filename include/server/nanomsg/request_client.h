/*!
    \file request_client.h
    \brief Nanomsg request client definition
    \author Ivan Shynkarenka
    \date 02.02.2017
    \copyright MIT License
*/

#ifndef CPPSERVER_NANOMSG_REQUEST_CLIENT_H
#define CPPSERVER_NANOMSG_REQUEST_CLIENT_H

#include "client.h"

namespace CppServer {
namespace Nanomsg {

//! Nanomsg request client
/*!
    Nanomsg request client is used to send requests to the Nanomsg reply server
    and receive corresponding responses from it.

    This protocol is used to distribute the workload among multiple stateless
    workers.

    Please note that request/reply applications should be stateless.

    It's important to include all the information necessary to process the
    request in the request itself, including information about the sender or
    the originator of the request if this is necessary to respond to the request.

    Sender information cannot be retrieved from the underlying socket connection
    since, firstly, transports like IPC may not have a firm notion of a message
    origin. Secondly, transports that have some notion may not have a reliable one
    - a TCP disconnect may mean a new sender, or it may mean a temporary loss in
    network connectivity.

    For this reason, sender information must be included by the application if
    required. Allocating 6 randomly-generated bytes in the message for the lifetime
    of the connection is sufficient for most purposes. For longer-lived
    applications, an UUID is more suitable.

    Thread-safe.
*/
class RequestClient : public Client
{
public:
    //! Initialize client with a given endpoint address
    /*!
        \param address - Endpoint address
        \param threading - Run the client in a separate thread (default is false)
    */
    explicit RequestClient(const std::string& address, bool threading = false)
        : Client(CppServer::Nanomsg::Domain::Std, CppServer::Nanomsg::Protocol::Request, address, threading)
    {}
    RequestClient(const RequestClient&) = delete;
    RequestClient(RequestClient&&) = default;
    virtual ~RequestClient() = default;

    RequestClient& operator=(const RequestClient&) = delete;
    RequestClient& operator=(RequestClient&&) = default;

    //! Request data to the server
    /*!
        \param buffer - Buffer to send
        \param size - Buffer size
        \return Reply message
    */
    Message Request(const void* buffer, size_t size)
    {
        Message message;
        if (Send(buffer, size) == size)
            Receive(message);
        return message;
    }
    //! Request a text string to the server
    /*!
        \param text - Text string to send
        \return Reply message
    */
    Message Request(const std::string& text) { return Request(text.data(), text.size()); }
    //! Request a message to the server
    /*!
        \param message - Message to send
        \return Reply message
    */
    Message Request(const Message& message) { return Request(message.buffer(), message.size()); }
};

/*! \example nanomsg_request_client.cpp Nanomsg request client example */

} // namespace Nanomsg
} // namespace CppServer

#endif // CPPSERVER_NANOMSG_REQUEST_CLIENT_H
