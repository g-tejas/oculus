#include <boost/process.hpp>
#include <cstdlib>
#include <iostream>
#include <simdjson.h>
#include <stdio.h>

using namespace boost::process;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << "<$YABAI_WINDOW_ID>\n";
    return EXIT_FAILURE;
  }

  ipstream pipe_stream;
  child c("gcc --version", std_out > pipe_stream);

  std::string line;

  while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
    std::cerr << line << std::endl;

  c.wait();

  return 0;
}
