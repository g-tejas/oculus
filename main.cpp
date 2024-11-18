#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <simdjson.h>
#include <string>
#include <vector>

namespace bp = boost::process;
namespace fs = std::filesystem;
using namespace simdjson;

struct Session {
  int64_t window_id;
  std::string app;
  std::string title;
  double start_time;
  double end_time;
  double duration;
};

struct SessionsData {
  std::optional<Session> current_session;
  std::vector<Session> sessions;
};

// Get the path to the lock file
fs::path get_lock_path() {
  return fs::path(getenv("HOME")) / ".oculus_sessions.lock";
}

// Get the path to the sessions file
fs::path get_sessions_path() {
  return fs::path(getenv("HOME")) / ".oculus_sessions.json";
}

// Execute yabai command and get windows data
simdjson::ondemand::parser &get_windows() {
  bp::ipstream pipe_stream;
  bp::child c("yabai -m query --windows", bp::std_out > pipe_stream);

  std::string output;
  std::string line;
  while (pipe_stream && std::getline(pipe_stream, line)) {
    output += line;
  }
  c.wait();

  static simdjson::ondemand::parser parser;
  return parser;
}

// Load sessions from file
SessionsData load_sessions() {
  SessionsData data;
  auto sessions_file = get_sessions_path();

  boost::interprocess::file_lock flock(get_lock_path().c_str());
  flock.lock();

  if (!fs::exists(sessions_file)) {
    data.current_session = std::nullopt;
    flock.unlock();
    return data;
  }

  std::ifstream file(sessions_file);
  std::string json_str((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  simdjson::ondemand::parser parser;
  auto json = parser.parse(json_str);

  // Parse the JSON structure
  auto current = json["current_session"];
  if (!current.is_null()) {
    Session session;
    session.window_id = current["window_id"];
    session.app = std::string(current["app"]);
    session.title = std::string(current["title"]);
    session.start_time = double(current["start_time"]);
    data.current_session = session;
  }

  auto sessions = json["sessions"];
  for (auto session : sessions) {
    Session s;
    s.window_id = session["window_id"];
    s.app = std::string(session["app"]);
    s.title = std::string(session["title"]);
    s.start_time = double(session["start_time"]);
    s.end_time = double(session["end_time"]);
    s.duration = double(session["duration"]);
    data.sessions.push_back(s);
  }

  flock.unlock();
  return data;
}

// Save sessions to file
void save_sessions(const SessionsData &data) {
  auto sessions_file = get_sessions_path();
  boost::interprocess::file_lock flock(get_lock_path().c_str());
  flock.lock();

  std::ofstream file(sessions_file);
  file << "{\n  \"current_session\": ";

  if (data.current_session) {
    const auto &curr = *data.current_session;
    file << "{\n    \"window_id\": " << curr.window_id << ",\n    \"app\": \""
         << curr.app << "\",\n    \"title\": \"" << curr.title
         << "\",\n    \"start_time\": " << curr.start_time << "\n  }";
  } else {
    file << "null";
  }

  file << ",\n  \"sessions\": [\n";
  for (size_t i = 0; i < data.sessions.size(); ++i) {
    const auto &s = data.sessions[i];
    file << "    {\n      \"window_id\": " << s.window_id
         << ",\n      \"app\": \"" << s.app << "\",\n      \"title\": \""
         << s.title << "\",\n      \"start_time\": " << s.start_time
         << ",\n      \"end_time\": " << s.end_time
         << ",\n      \"duration\": " << s.duration << "\n    }"
         << (i < data.sessions.size() - 1 ? "," : "") << "\n";
  }
  file << "  ]\n}";

  flock.unlock();
}

void update_session(int64_t window_id) {
  auto current_time =
      std::chrono::system_clock::now().time_since_epoch().count() / 1e9;
  auto sessions_data = load_sessions();

  if (sessions_data.current_session) {
    auto &current = *sessions_data.current_session;
    Session completed_session = current;
    completed_session.end_time = current_time;
    completed_session.duration = current_time - current.start_time;
    sessions_data.sessions.push_back(completed_session);
    sessions_data.current_session = std::nullopt;
  }

  if (window_id == -1) {
    save_sessions(sessions_data);
    return;
  }

  auto parser = get_windows();
  auto windows = parser.parse(""); // Your JSON string from yabai
  bool found = false;

  for (auto window : windows) {
    if (int64_t(window["id"]) == window_id) {
      Session new_session;
      new_session.window_id = window_id;
      new_session.app = std::string(window["app"]);
      new_session.title = std::string(window["title"]);
      new_session.start_time = current_time;
      sessions_data.current_session = new_session;
      found = true;
      break;
    }
  }

  if (!found) {
    std::cerr << "Window with id " << window_id << " not found\n";
    return;
  }

  save_sessions(sessions_data);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <window_id>\n";
    return 1;
  }

  int64_t window_id = std::stoll(argv[1]);
  update_session(window_id);
  return 0;
}
