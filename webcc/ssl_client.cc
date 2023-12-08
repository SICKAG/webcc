#include "webcc/ssl_client.h"

#include "boost/asio/ssl.hpp"
#include "boost/asio/write.hpp"

#include "webcc/logger.h"

using namespace std::placeholders;
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

namespace webcc {

SslClient::SslClient(boost::asio::io_context& io_context,
                     boost::asio::ssl::context& ssl_context,
                     SslVerify ssl_verify)
    : ClientBase(io_context, "443"),
      ssl_stream_(io_context, ssl_context),
      ssl_verify_(ssl_verify),
      ssl_shutdown_timer_(io_context) {
}

bool SslClient::Close() {
  bool new_async_op = false;
  SocketType& socket = GetSocket();

  if (socket.is_open()) {
    SocketCancel(socket);

    if (connected_) {
      connected_ = false;

      if (hand_shaken_) {
        // SSL shudown is necessary only if handshake has completed.
        LOG_INFO("Shut down SSL...");

        // Stop the timer for connect, write or read operation.
        StopDeadlineTimer("close");

        LOG_INFO("Start ssl shutdown timer (%ds)", ssl_shutdown_timeout_);
        ssl_shutdown_timer_active_ = true;
        ssl_shutdown_timer_.expires_after(
            boost::asio::chrono::seconds(ssl_shutdown_timeout_));
        ssl_shutdown_timer_.async_wait(
            std::bind(&SslClient::OnSslShutdownTimer, shared_from_this(), _1));

        ssl_stream_.async_shutdown(
            std::bind(&SslClient::OnSslShutdown, shared_from_this(), _1));

        new_async_op = true;

      } else {
        SocketShutdownClose(socket);
      }
    } else {
      SocketClose(socket);
    }
  } else {
    // TODO: resolver_.cancel() ?
  }

  return new_async_op;
}

void SslClient::AsyncWrite(
    const std::vector<boost::asio::const_buffer>& buffers,
    AsyncRWHandler&& handler) {
  boost::asio::async_write(ssl_stream_, buffers, std::move(handler));
}

void SslClient::AsyncReadSome(boost::asio::mutable_buffer buffer,
                              AsyncRWHandler&& handler) {
  ssl_stream_.async_read_some(buffer, std::move(handler));
}

void SslClient::OnConnected() {
  const std::string& host = request_->host();

  // Modes `ssl::verify_fail_if_no_peer_cert` and `ssl::verify_client_once` are
  // for server only while mode `ssl::verify_none` is not secure.
  // See: https://stackoverflow.com/a/12621528
  ssl_stream_.set_verify_mode(ssl::verify_peer);

  if (ssl_verify_ == SslVerify::kHostName) {
    // Set SNI (server name indication) host name.
    // Many hosts need this to handshake successfully (e.g., google.com).
    // Inspired by Boost.Beast.
    if (!SSL_set_tlsext_host_name(ssl_stream_.native_handle(), host.c_str())) {
      LOG_ERRO("Failed to set SNI host name for SSL");
    }

    ssl_stream_.set_verify_callback(ssl::host_name_verification{ host });

  } else {
    // Don't set a verify callback.
  }

  auto self = std::dynamic_pointer_cast<SslClient>(shared_from_this());

  ssl_stream_.async_handshake(ssl::stream_base::client,
                              std::bind(&SslClient::OnHandshake, self, _1));
}

void SslClient::OnHandshake(boost::system::error_code ec) {
  if (ec) {
    LOG_ERRO("Handshake error (%s)", ec.message().c_str());
    Close();
    error_.Set(error_codes::kHandshakeError, "Handshake error");
    return;
  }

  LOG_INFO("Handshake OK");
  hand_shaken_ = true;

  ClientBase::AsyncWrite();
}

void SslClient::OnSslShutdownTimer(boost::system::error_code ec) {
  if (ec == boost::asio::error::operation_aborted) {
    LOG_INFO("SSL shutdown timer canceled");
    return;
  }

  LOG_INFO("Cancel the SSL shutdown");

  GetSocket().cancel(ec);
  if (ec) {
    LOG_WARN("Socket cancel error (%s)", ec.message().c_str());
    ec.clear();
  }
}

void SslClient::OnSslShutdown(boost::system::error_code ec) {
  StopSslShutdownTimer();

  // See: https://stackoverflow.com/a/25703699
  if (ec == boost::asio::error::eof) {
    ec = {};  // Not an error
  }

  if (ec) {
    // Failed or canceled by the deadline timer.
    LOG_WARN("SSL shutdown error (%s)", ec.message().c_str());
  } else {
    LOG_INFO("SSL shutdown complete");
  }

  // Continue to shut down and close the socket.
  SocketShutdownClose(GetSocket());
}

void SslClient::StopSslShutdownTimer() {
  if (!ssl_shutdown_timer_active_) {
    return;
  }

  ssl_shutdown_timer_active_ = false;

  LOG_INFO("Cancel ssl shutdown timer");

  try {
    ssl_shutdown_timer_.cancel();

  } catch (const boost::system::system_error& e) {
    LOG_ERRO("Ssl shutdown timer cancel error: %s", e.what());
  }
}

}  // namespace webcc
