/*!
    \file ssl_session.h
    \brief SSL session definition
    \author Ivan Shynkarenka
    \date 30.12.2016
    \copyright MIT License
*/

#ifndef CPPSERVER_ASIO_SSL_SESSION_H
#define CPPSERVER_ASIO_SSL_SESSION_H

#include "tcp_session.h"

namespace CppServer {
namespace Asio {

template <class TServer, class TSession>
class SSLServer;

//! SSL session
/*!
    SSL session is used to read and write data from the connected SSL client.

    Thread-safe.
*/
template <class TServer, class TSession>
class SSLSession : public std::enable_shared_from_this<SSLSession<TServer, TSession>>
{
    template <class TSomeServer, class TSomeSession>
    friend class SSLServer;

public:
    //! Initialize the session with a given server, socket and SSL context
    /*!
        \param server - Connected server
        \param socket - Connected socket
        \param context - SSL context
    */
    explicit SSLSession(std::shared_ptr<SSLServer<TServer, TSession>> server, asio::ip::tcp::socket&& socket, std::shared_ptr<asio::ssl::context> context);
    SSLSession(const SSLSession&) = delete;
    SSLSession(SSLSession&&) = default;
    virtual ~SSLSession() = default;

    SSLSession& operator=(const SSLSession&) = delete;
    SSLSession& operator=(SSLSession&&) = default;

    //! Get the session Id
    const CppCommon::UUID& id() const noexcept { return _id; }

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _server->service(); }
    //! Get the session server
    std::shared_ptr<SSLServer<TServer, TSession>>& server() noexcept { return _server; }
    //! Get the session SSL stream
    asio::ssl::stream<asio::ip::tcp::socket>& stream() noexcept { return _stream; }
    //! Get the session socket
    asio::ssl::stream<asio::ip::tcp::socket>::lowest_layer_type& socket() noexcept { return _stream.lowest_layer(); }
    //! Get the session SSL context
    std::shared_ptr<asio::ssl::context>& context() noexcept { return _context; }

    //! Get the number of bytes sent by this session
    uint64_t bytes_sent() const noexcept { return _bytes_sent; }
    //! Get the number of bytes received by this session
    uint64_t bytes_received() const noexcept { return _bytes_received; }

    //! Is the session connected?
    bool IsConnected() const noexcept { return _connected; }
    //! Is the session handshaked?
    bool IsHandshaked() const noexcept { return _handshaked; }

    //! Disconnect the session
    /*!
        \return 'true' if the section was successfully disconnected, 'false' if the section is already disconnected
    */
    bool Disconnect() { return Disconnect(false); }

    //! Send data into the session
    /*!
        \param buffer - Buffer to send
        \param size - Buffer size
        \return Count of pending bytes in the send buffer
    */
    size_t Send(const void* buffer, size_t size);
    //! Send a text string into the session
    /*!
        \param text - Text string to send
        \return Count of pending bytes in the send buffer
    */
    size_t Send(const std::string& text) { return Send(text.data(), text.size()); }

protected:
    //! Handle session connected notification
    virtual void onConnected() {}
    //! Handle session handshaked notification
    virtual void onHandshaked() {}
    //! Handle session disconnected notification
    virtual void onDisconnected() {}

    //! Handle buffer received notification
    /*!
        Notification is called when another chunk of buffer was received
        from the client.

        \param buffer - Received buffer
        \param size - Received buffer size
    */
    virtual void onReceived(const void* buffer, size_t size) {}
    //! Handle buffer sent notification
    /*!
        Notification is called when another chunk of buffer was sent
        to the client.

        This handler could be used to send another buffer to the client
        for instance when the pending size is zero.

        \param sent - Size of sent buffer
        \param pending - Size of pending buffer
    */
    virtual void onSent(size_t sent, size_t pending) {}

    //! Handle empty send buffer notification
    /*!
        Notification is called when the send buffer is empty and ready
        for a new data to send.

        This handler could be used to send another buffer to the client.
    */
    virtual void onEmpty() {}

    //! Handle error notification
    /*!
        \param error - Error code
        \param category - Error category
        \param message - Error message
    */
    virtual void onError(int error, const std::string& category, const std::string& message) {}

private:
    // Session Id
    CppCommon::UUID _id;
    // Session server, SSL stream and SSL context
    std::shared_ptr<SSLServer<TServer, TSession>> _server;
    asio::ssl::stream<asio::ip::tcp::socket> _stream;
    std::shared_ptr<asio::ssl::context> _context;
    std::atomic<bool> _connected;
    std::atomic<bool> _handshaked;
    // Session statistic
    uint64_t _bytes_sent;
    uint64_t _bytes_received;
    // Receive buffer & cache
    bool _reciving;
    std::vector<uint8_t> _recive_buffer;
    // Send buffer & cache
    bool _sending;
    std::mutex _send_lock;
    std::vector<uint8_t> _send_buffer_main;
    std::vector<uint8_t> _send_buffer_flush;
    size_t _send_buffer_flush_offset;

    //! Connect the session
    void Connect();
    //! Disconnect the session
    /*!
        \param dispatch - Dispatch flag
        \return 'true' if the session was successfully disconnected, 'false' if the session is already disconnected
    */
    bool Disconnect(bool dispatch);

    //! Try to receive new data
    void TryReceive();
    //! Try to send pending data
    void TrySend();

    //! Clear receive & send buffers
    void ClearBuffers();

    //! Send error notification
    void SendError(std::error_code ec);
};

} // namespace Asio
} // namespace CppServer

#include "ssl_session.inl"

#endif // CPPSERVER_ASIO_SSL_SESSION_H
