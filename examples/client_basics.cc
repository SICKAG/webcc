#include <cassert>
#include <iostream>

#include "webcc/client_session.h"
#include "webcc/logger.h"

int main() {
  WEBCC_LOG_INIT("", webcc::LOG_CONSOLE);

  webcc::ClientSession session;

  // The following settings are optional.
  // They'll be applied to all the requests on this session.
  session.set_connect_timeout(5);
  session.set_read_timeout(5);
  session.Accept("application/json");

  webcc::ResponsePtr r;

  try {
    r = session.Send(WEBCC_GET("http://httpbin.org/get")
                         .Query("name", "Adam Gu", true)
                         .Date()());

    r = session.Send(WEBCC_POST("http://httpbin.org/post")
                         .Body("{'name'='Adam', 'age'=20}")
                         .Json()
                         .Utf8()());

    r = session.Send(WEBCC_GET("https://httpbin.org/get")());

  } catch (const webcc::Error& error) {
    std::cerr << error << std::endl;
    return 1;
  }

  return 0;
}
