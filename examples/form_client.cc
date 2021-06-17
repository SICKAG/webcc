// A client posting multipart form data.

#include <iostream>

#include "webcc/client_session.h"
#include "webcc/fs.h"
#include "webcc/logger.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: form_client <upload_dir> [url]" << std::endl;
    std::cout << std::endl;
    std::cout << "Default url: http://httpbin.org/post" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "(Post to httpbin.org)" << std::endl;
    std::cout << "  $ ./form_client path/to/webcc/data/upload" << std::endl;
    std::cout << "  $ ./form_client path/to/webcc/data/upload "
              << "http://httpbin.org/post" << std::endl;
    std::cout << "(Post to the example 'form_server')" << std::endl;
    std::cout << "  $ ./form_client path/to/webcc/data/upload "
                 "http://localhost:8080/upload"
              << std::endl;
    return 1;
  }

  WEBCC_LOG_INIT("", webcc::LOG_CONSOLE);

  const webcc::fs::path upload_dir(argv[1]);

  std::string url;
  if (argc == 3) {
    url = argv[2];
  } else {
    url = "http://httpbin.org/post";
  }

  if (!webcc::fs::is_directory(upload_dir) || !webcc::fs::exists(upload_dir)) {
    std::cerr << "Invalid upload dir!" << std::endl;
    return 1;
  }

  webcc::ClientSession session;

  try {
    auto r = session.Send(WEBCC_POST(url)
                              .FormFile("file", upload_dir / "remember.txt")
                              .FormData("json", "{}", "application/json")());

    std::cout << r->status() << std::endl;

  } catch (const webcc::Error& error) {
    std::cout << error << std::endl;
  }

  return 0;
}
