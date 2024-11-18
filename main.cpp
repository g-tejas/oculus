#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace bp = boost::process;
namespace fs = std::filesystem;
using json = nlohmann::json;

using seconds_t = std::chrono::seconds;

auto get_seconds_since_epoch() -> decltype(seconds_t().count()) {
  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  return seconds.count();
}

struct Session {
  int64_t id;
  std::string app;
  std::string title;
  seconds_t start_time;
  seconds_t end_time;
};

auto get_windows_json() -> json {
  bp::ipstream pipe_stream;
  bp::child c("yabai -m query --windows", bp::std_out > pipe_stream);

  std::string output;
  std::string line;
  while (pipe_stream && std::getline(pipe_stream, line)) {
    output += line;
  }
  c.wait();

  return json::parse(output);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <window_id>\n";
    return 1;
  }

  std::string window_id_str(argv[1]);

  auto windows = get_windows_json();
  std::cout << std::setw(4) << windows << std::endl;

  for (const auto &window : windows) {
    if (window["id"] == window_id_str) {
      std::cout << "Window found: " << window["id"] << std::endl;
    }
  }

  return 0;
}
