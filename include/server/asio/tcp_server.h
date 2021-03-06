/*!
    \file tcp_server.h
    \brief TCP server definition
    \author Ivan Shynkarenka
    \date 14.12.2016
    \copyright MIT License
*/

#ifndef CPPSERVER_ASIO_TCP_SERVER_H
#define CPPSERVER_ASIO_TCP_SERVER_H

#include "tcp_session.h"

#include <map>
#include <mutex>
#include <vector>

namespace CppServer {
namespace Asio {

template <class TServer, class TSession>
class TCPSession;

//! TCP server
/*!
    TCP server is used to connect, disconnect and manage TCP sessions.

    Thread-safe.
*/
template <class TServer, class TSession>
class TCPServer : public std::enable_shared_from_this<TCPServer<TServer, TSession>>
{
    template <class TSomeServer, class TSomeSession>
    friend class TCPSession;

public:
    //! Initialize TCP server with a given Asio service, protocol and port number
    /*!
        \param service - Asio service
        \param protocol - Protocol type
        \param port - Port number
    */
    explicit TCPServer(std::shared_ptr<Service> service, InternetProtocol protocol, int port);
    //! Initialize TCP server with a given Asio service, IP address and port number
    /*!
        \param service - Asio service
        \param address - IP address
        \param port - Port number
    */
    explicit TCPServer(std::shared_ptr<Service> service, const std::string& address, int port);
    //! Initialize TCP server with a given Asio service and endpoint
    /*!
        \param service - Asio service
        \param endpoint - Server TCP endpoint
    */
    explicit TCPServer(std::shared_ptr<Service> service, const asio::ip::tcp::endpoint& endpoint);
    TCPServer(const TCPServer&) = delete;
    TCPServer(TCPServer&&) = default;
    virtual ~TCPServer() = default;

    TCPServer& operator=(const TCPServer&) = delete;
    TCPServer& operator=(TCPServer&&) = default;

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _service; }
    //! Get the server endpoint
    asio::ip::tcp::endpoint& endpoint() noexcept { return _endpoint; }
    //! Get the server acceptor
    asio::ip::tcp::acceptor& acceptor() noexcept { return _acceptor; }

    //! Get the number of sessions currently connected to this server
    uint64_t current_sessions() const noexcept { return _sessions.size(); }
    //! Get the number of bytes sent by this server
    uint64_t bytes_sent() const noexcept { return _bytes_sent; }
    //! Get the number of bytes received by this server
    uint64_t bytes_received() const noexcept { return _bytes_received; }

    //! Is the server started?
    bool IsStarted() const noexcept { return _started; }

    //! Start the server
    /*!
        \return 'true' if the server was successfully started, 'false' if the server failed to start
    */
    bool Start();
    //! Stop the server
    /*!
        \return 'true' if the server was successfully stopped, 'false' if the server is already stopped
    */
    bool Stop();
    //! Restart the server
    /*!
        \return 'true' if the server was successfully restarted, 'false' if the server failed to restart
    */
    bool Restart();

    //! Multicast data to all connected sessions
    /*!
        \param buffer - Buffer to multicast
        \param size - Buffer size
        \return 'true' if the data was successfully multicast, 'false' if the server it not started
    */
    bool Multicast(const void* buffer, size_t size);
    //! Multicast a text string to all connected sessions
    /*!
        \param text - Text string to multicast
        \return 'true' if the text string was successfully multicast, 'false' if the server it not started
    */
    bool Multicast(const std::string& text) { return Multicast(text.data(), text.size()); }

    //! Disconnect all connected sessions
    /*!
        \return 'true' if all sessions were successfully disconnected, 'false' if the server it not started
    */
    bool DisconnectAll();

protected:
    //! Handle server started notification
    virtual void onStarted() {}
    //! Handle server stopped notification
    virtual void onStopped() {}

    //! Handle new session connected notification
    /*!
        \param session - Connected session
    */
    virtual void onConnected(std::shared_ptr<TSession>& session) {}
    //! Handle session disconnected notification
    /*!
        \param session - Disconnected session
    */
    virtual void onDisconnected(std::shared_ptr<TSession>& session) {}

    //! Handle error notification
    /*!
        \param error - Error code
        \param category - Error category
        \param message - Error message
    */
    virtual void onError(int error, const std::string& category, const std::string& message) {}

private:
    // Asio service
    std::shared_ptr<Service> _service;
    // Server endpoint, acceptor & socket
    asio::ip::tcp::endpoint _endpoint;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;
    std::atomic<bool> _started;
    // Server statistic
    uint64_t _bytes_sent;
    uint64_t _bytes_received;
    // Server sessions
    std::map<CppCommon::UUID, std::shared_ptr<TSession>> _sessions;
    // Multicast buffer
    std::mutex _multicast_lock;
    std::vector<uint8_t> _multicast_buffer;

    //! Accept new connections
    void Accept();

    //! Register a new session
    std::shared_ptr<TSession> RegisterSession();
    //! Unregister the given session
    /*!
        \param id - Session Id
    */
    void UnregisterSession(const CppCommon::UUID& id);

    //! Clear multicast buffer
    void ClearBuffers();

    //! Send error notification
    void SendError(std::error_code ec);
};

/*! \example tcp_chat_server.cpp TCP chat server example */

} // namespace Asio
} // namespace CppServer

#include "tcp_server.inl"

#endif // CPPSERVER_ASIO_TCP_SERVER_H
