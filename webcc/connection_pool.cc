#include "webcc/connection_pool.h"

#include "webcc/logger.h"

namespace webcc {

void ConnectionPool::Start(ConnectionPtr connection) {
  LOG_VERB("Start connection");

  {
    // Lock the container only.
    std::lock_guard<std::mutex> lock{ mutex_ };
    connections_.insert(connection);
  }

  connection->Start();
}

void ConnectionPool::Close(ConnectionPtr connection) {
  // NOTE:
  // The connection might have already been closed by Clear().

  std::lock_guard<std::mutex> lock{ mutex_ };

  // Check the return value of erase() to see if it still exists or not.
  if (connections_.erase(connection) == 1) {
    LOG_VERB("Close connection");
    connection->Close();
  }  // else: Already closed by Clear()
}

void ConnectionPool::Clear() {
  // Lock all since we are going to stop anyway.
  std::lock_guard<std::mutex> lock{ mutex_ };

  if (!connections_.empty()) {
    LOG_VERB("Close all (%u) connections", connections_.size());
    for (auto& c : connections_) {
      c->Close();
    }

    // Closing the connection will not cancel any pending SSL operations (handshake).
    // If the connection gets destructed before the handler of such an operation completes, it can cause a segfault.
    // See also: https://github.com/chriskohlhoff/asio/issues/355
    Sleep(500);

    connections_.clear();
  }
}

}  // namespace webcc
