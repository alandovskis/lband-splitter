#include "logger.h"

#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

namespace splitter::utils {

std::shared_ptr<spdlog::logger> Logger::logger_;
std::string Logger::current_log_file_;

bool Logger::init(const std::string& log_file_path, 
                 const std::string& log_level,
                 size_t max_file_size_mb,
                 size_t max_files) {
    
    try {
        std::filesystem::create_directories(std::filesystem::path(log_file_path).parent_path());
        
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        
        size_t max_file_size_bytes = max_file_size_mb * 1024 * 1024;
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file_path, max_file_size_bytes, max_files
        );
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");
        
        logger_ = std::make_shared<spdlog::logger>("splitter_daemon",
            spdlog::sinks_init_list{console_sink, file_sink});
        
        logger_->set_level(string_to_level(log_level));
        logger_->flush_on(spdlog::level::warn);
        
        spdlog::set_default_logger(logger_);
        spdlog::set_automatic_registration(true);
        
        current_log_file_ = log_file_path;
        
        info("Logger initialized - file: {}, level: {}, max_size: {}MB, max_files: {}",
             log_file_path, log_level, max_file_size_mb, max_files);
        
        return true;
        
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Failed to initialize logger: %s\n", e.what());
        return false;
    }
}

void Logger::shutdown() {
    if (logger_) {
        info("Logger shutting down");
        logger_->flush();
        spdlog::shutdown();
        logger_.reset();
    }
}

void Logger::set_level(const std::string& level) {
    if (logger_) {
        auto spdlog_level = string_to_level(level);
        logger_->set_level(spdlog_level);
        info("Log level changed to: {}", level);
    }
}

std::string Logger::get_level() {
    if (logger_) {
        return level_to_string(logger_->level());
    }
    return "info";
}

bool Logger::is_initialized() {
    return logger_ != nullptr;
}

spdlog::level::level_enum Logger::string_to_level(const std::string& level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "info") return spdlog::level::info;
    if (level == "warning" || level == "warn") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    if (level == "off") return spdlog::level::off;
    
    return spdlog::level::info;
}

std::string Logger::level_to_string(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace: return "trace";
        case spdlog::level::debug: return "debug";
        case spdlog::level::info: return "info";
        case spdlog::level::warn: return "warning";
        case spdlog::level::err: return "error";
        case spdlog::level::critical: return "critical";
        case spdlog::level::off: return "off";
        default: return "info";
    }
}

} // namespace splitter::utils