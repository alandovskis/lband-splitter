#pragma once

#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>

namespace splitter::utils {

class Logger {
public:
  static bool init(const std::string &log_file_path,
                   const std::string &log_level = "info",
                   size_t max_file_size_mb = 100, size_t max_files = 10);

  static void shutdown();

  template <typename... Args>
  static void trace(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->trace(format, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  static void debug(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->debug(format, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  static void info(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->info(format, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  static void warning(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->warn(format, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  static void error(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->error(format, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  static void critical(const std::string &format, Args &&...args) {
    if (logger_) {
      logger_->critical(format, std::forward<Args>(args)...);
    }
  }

  static void set_level(const std::string &level);
  static std::string get_level();

  static bool is_initialized();

private:
  static std::shared_ptr<spdlog::logger> logger_;
  static std::string current_log_file_;
  static spdlog::level::level_enum string_to_level(const std::string &level);
  static std::string level_to_string(spdlog::level::level_enum level);
};

} // namespace splitter::utils