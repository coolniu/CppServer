/*!
    \file websocket_ssl_client.cpp
    \brief WebSocket SSL client implementation
    \author Ivan Shynkarenka
    \date 11.01.2016
    \copyright MIT License
*/

#include "server/asio/websocket_ssl_client.h"

namespace CppServer {
namespace Asio {

WebSocketSSLClient::WebSocketSSLClient(std::shared_ptr<Service> service, std::shared_ptr<asio::ssl::context> context, const std::string& uri)
    : _id(CppCommon::UUID::Generate()),
      _service(service),
      _context(context),
      _uri(uri),
      _initialized(false),
      _connected(false),
      _messages_sent(0),
      _messages_received(0),
      _bytes_sent(0),
      _bytes_received(0)
{
    assert((service != nullptr) && "ASIO service is invalid!");
    if (service == nullptr)
        throw CppCommon::ArgumentException("ASIO service is invalid!");

    assert((context != nullptr) && "SSL context is invalid!");
    if (context == nullptr)
        throw CppCommon::ArgumentException("SSL context is invalid!");

    InitAsio();
}

void WebSocketSSLClient::InitAsio()
{
    assert(!_initialized && "Asio is already initialed!");
    if (_initialized)
        return;

    // Setup WebSocket client core Asio service
    websocketpp::lib::error_code ec;
    _core.init_asio(_service->service().get(), ec);
    if (ec)
    {
        SendError(ec);
        return;
    }

    _initialized = true;
}

bool WebSocketSSLClient::Connect()
{
    assert(_initialized && "Asio is not initialed!");
    if (!_initialized)
        return false;

    if (IsConnected())
        return false;

    // Post the connect routine
    auto self(this->shared_from_this());
    _service->service()->post([this, self]()
    {
        websocketpp::lib::error_code ec;

        // Setup WebSocket client core logging
        _core.set_access_channels(websocketpp::log::alevel::none);
        _core.set_error_channels(websocketpp::log::elevel::none);

        // Setup WebSocket server core handlers
        _core.set_open_handler([this](websocketpp::connection_hdl connection) { Connected(connection); });
        _core.set_close_handler([this](websocketpp::connection_hdl connection) { Disconnected(connection); });
        _core.set_tls_init_handler([this](websocketpp::connection_hdl connection) { return _context; });

        // Get the client connection
        WebSocketSSLClientCore::connection_ptr connection_ptr = _core.get_connection(_uri, ec);
        if (ec)
        {
            SendError(ec);
            onDisconnected();
            return;
        }

        // Setup WebSocket client handlers
        connection_ptr->set_message_handler([this](websocketpp::connection_hdl connection, WebSocketSSLMessage message)
        {
            size_t size = message->get_raw_payload().size();

            // Update statistic
            ++_messages_received;
            _bytes_received += size;

            // Call the message received handler
            onReceived(message);
        });
        connection_ptr->set_fail_handler([this](websocketpp::connection_hdl connection)
        {
            WebSocketSSLServerCore::connection_ptr con = _core.get_con_from_hdl(connection);
            websocketpp::lib::error_code ec = con->get_ec();
            SendError(ec);
            Disconnected(connection);
        });

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        _core.connect(connection_ptr);
    });

    return true;
}

void WebSocketSSLClient::Connected(websocketpp::connection_hdl connection)
{
    // Reset statistic
    _messages_sent = 0;
    _messages_received = 0;
    _bytes_sent = 0;
    _bytes_received = 0;

    // Update the connected state
    _connection = connection;
    _connected = true;

    // Call the client connected handler
    onConnected();
}

bool WebSocketSSLClient::Disconnect(bool dispatch, websocketpp::close::status::value code, const std::string& reason)
{
    if (!IsConnected())
        return false;

    auto self(this->shared_from_this());
    auto disconnect = [this, self, code, reason]()
    {
        // Close the client connection
        websocketpp::lib::error_code ec;
        _core.close(_connection, code, reason, ec);
        if (ec)
            SendError(ec);
    };

    // Dispatch or post the disconnect routine
    if (dispatch)
        _service->Dispatch(disconnect);
    else
        _service->Post(disconnect);

    return true;
}

void WebSocketSSLClient::Disconnected(websocketpp::connection_hdl connection)
{
    // Update the connected state
    _connection.reset();
    _connected = false;

    // Call the client disconnected handler
    onDisconnected();
}

bool WebSocketSSLClient::Reconnect()
{
    if (!Disconnect())
        return false;

    while (IsConnected())
        CppCommon::Thread::Yield();

    return Connect();
}

size_t WebSocketSSLClient::Send(const void* buffer, size_t size, websocketpp::frame::opcode::value opcode)
{
    assert((buffer != nullptr) && "Pointer to the buffer should not be equal to 'nullptr'!");
    assert((size > 0) && "Buffer size should be greater than zero!");
    if ((buffer == nullptr) || (size == 0))
        return 0;

    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, buffer, size, opcode, ec);
    if (ec)
    {
        SendError(ec);
        return 0;
    }

    // Update statistic
    ++_messages_sent;
    _bytes_sent += size;

    return size;
}

size_t WebSocketSSLClient::Send(const std::string& text, websocketpp::frame::opcode::value opcode)
{
    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, text, opcode, ec);
    if (ec)
    {
        SendError(ec);
        return 0;
    }

    size_t size = text.size();

    // Update statistic
    ++_messages_sent;
    _bytes_sent += size;

    return size;
}

size_t WebSocketSSLClient::Send(const WebSocketSSLMessage& message)
{
    if (!IsConnected())
        return 0;

    websocketpp::lib::error_code ec;
    _core.send(_connection, message, ec);
    if (ec)
    {
        SendError(ec);
        return 0;
    }

    size_t size = message->get_raw_payload().size();

    // Update statistic
    ++_messages_sent;
    _bytes_sent += size;

    return size;
}

void WebSocketSSLClient::SendError(std::error_code ec)
{
    onError(ec.value(), ec.category().name(), ec.message());
}

} // namespace Asio
} // namespace CppServer
