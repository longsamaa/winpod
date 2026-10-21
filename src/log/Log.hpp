#pragma once
#ifndef _WIN_POD_LOG_
#define _WIN_POD_LOG_
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace winpod {
class Log {
public:
  static Log &instance();

  Log(const Log &) = delete;
  Log &operator=(const Log &) = delete;

public:
  void init(const std::string& appName);
  void shutdown();
  std::shared_ptr<spdlog::logger> get(std::string_view name);
    std::shared_ptr<spdlog::logger> getAppLogger(); 
private:
  Log() = default;
  ~Log() = default;
  std::shared_ptr<spdlog::logger> create(const std::string &name);

private:
  std::string m_Path{"logs/app.log"};
  std::string m_AppName{""}; 
  std::mutex m_Mutex;
  std::vector<spdlog::sink_ptr> m_Sinks;
  bool m_Initialized{false};
};
} // namespace winpod
#endif //_WIN_POD_LOG_
