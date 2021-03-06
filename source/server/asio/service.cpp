/*!
    \file service.cpp
    \brief Asio service implementation
    \author Ivan Shynkarenka
    \date 16.12.2016
    \copyright MIT License
*/

#include "server/asio/service.h"

#include "errors/fatal.h"

namespace CppServer {
namespace Asio {

Service::Service()
    : _service(std::make_shared<asio::io_service>()),
      _started(false)
{
    assert((_service != nullptr) && "ASIO service is invalid!");
    if (_service == nullptr)
        throw CppCommon::ArgumentException("ASIO service is invalid!");
}

Service::Service(std::shared_ptr<asio::io_service> service)
    : _service(service),
      _started(false)
{
    assert((_service != nullptr) && "ASIO service is invalid!");
    if (_service == nullptr)
        throw CppCommon::ArgumentException("ASIO service is invalid!");

    _started = !service->stopped();
}

bool Service::Start(bool polling)
{
    assert(!IsStarted() && "Asio service is already started!");
    if (IsStarted())
        return false;

    // Post the started routine
    auto self(this->shared_from_this());
    _service->post([this, self]()
    {
        if (IsStarted())
            return;

         // Update the started flag
        _started = true;

        // Call the service started handler
        onStarted();
    });

    // Start the service thread
    _thread = CppCommon::Thread::Start([this, polling]() { ServiceLoop(polling); });

    return true;
}

bool Service::Stop()
{
    assert(IsStarted() && "Asio service is not started!");
    if (!IsStarted())
        return false;

    // Post the stop routine
    auto self(this->shared_from_this());
    _service->post([this, self]()
    {
        if (!IsStarted())
            return;

        // Stop the Asio service
        _service->stop();

        // Update the started flag
        _started = false;

        // Call the service stopped handler
        onStopped();
    });

    // Wait for service thread
    _thread.join();

    return true;
}

bool Service::Restart()
{
    if (!Stop())
        return false;

    return Start();
}

void Service::ServiceLoop(bool polling)
{
    // Call the initialize thread handler
    onThreadInitialize();

    try
    {
        asio::io_service::work work(*_service);

        // Service loop...
        do
        {
            // ...with handling some specific Asio errors
            try
            {
                if (polling)
                {
                    // Poll all pending handlers
                    _service->poll();

                    // Call the idle handler
                    onIdle();
                }
                else
                {
                    // Run all pending handlers
                    _service->run();
                    break;
                }
            }
            catch (asio::system_error& ex)
            {
                std::error_code ec = ex.code();

                // Skip Asio disconnect errors
                if (ec == asio::error::not_connected)
                    continue;

                throw;
            }
        } while (_started);
    }
    catch (asio::system_error& ex)
    {
        SendError(ex.code());
    }
    catch (std::exception& ex)
    {
        fatality(ex);
    }
    catch (...)
    {
        fatality("Asio service thread terminated!");
    }

    // Call the cleanup thread handler
    onThreadCleanup();
}

void Service::SendError(std::error_code ec)
{
    onError(ec.value(), ec.category().name(), ec.message());
}

} // namespace Asio
} // namespace CppServer
