#include <chrono>
#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
// #include <systemd/sd-daemon.h>  // Not available on macOS

#include "core/config_manager.h"
#include "core/splitter_manager.h"
// #include "netconf/netconf_server.h"  // Disabled - libnetconf2 not available
// #include "web/rest_server.h"         // Disabled - cpprest not available
#include "utils/logger.h"
#include "utils/system_monitor.h"

namespace {
std::unique_ptr<splitter::core::SplitterManager> g_splitter_manager;
// std::unique_ptr<splitter::netconf::NetconfServer> g_netconf_server;  //
// Disabled std::unique_ptr<splitter::web::RestServer> g_rest_server; //
// Disabled
std::unique_ptr<splitter::utils::SystemMonitor> g_system_monitor;
volatile sig_atomic_t g_shutdown_requested = 0;
} // namespace

void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    g_shutdown_requested = 1;
    splitter::utils::Logger::info("Shutdown requested via signal {}", signum);
  }
}

void setup_signal_handlers() {
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
}

int main(int argc, char *argv[]) {
  try {
    splitter::utils::Logger::init("/tmp/splitter_daemon.log");
    splitter::utils::Logger::info("Starting L-band Splitter Daemon v{}",
                                  "1.0.0");

    setup_signal_handlers();

    auto config_manager = std::make_unique<splitter::core::ConfigManager>(
        argc > 1 ? argv[1] : "/etc/splitter/config.yaml");

    g_splitter_manager =
        std::make_unique<splitter::core::SplitterManager>(config_manager.get());

    // g_netconf_server = std::make_unique<splitter::netconf::NetconfServer>(
    //     g_splitter_manager.get()
    // );  // Disabled - libnetconf2 not available

    // g_rest_server = std::make_unique<splitter::web::RestServer>(
    //     g_splitter_manager.get(),
    //     config_manager->get_web_port()
    // );  // Disabled - cpprest not available

    g_system_monitor = std::make_unique<splitter::utils::SystemMonitor>(
        g_splitter_manager.get());

    if (!g_splitter_manager->initialize()) {
      splitter::utils::Logger::error("Failed to initialize splitter manager");
      return 1;
    }

    // if (!g_netconf_server->start()) {
    //     splitter::utils::Logger::error("Failed to start NetConf server");
    //     return 1;
    // }  // Disabled - libnetconf2 not available

    // if (!g_rest_server->start()) {
    //     splitter::utils::Logger::error("Failed to start REST API server");
    //     return 1;
    // }  // Disabled - cpprest not available

    g_system_monitor->start();

    // sd_notify(0, "READY=1");  // systemd not available on macOS
    splitter::utils::Logger::info(
        "Daemon initialized successfully, entering main loop");

    while (!g_shutdown_requested) {
      g_splitter_manager->process_events();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    splitter::utils::Logger::info("Shutting down daemon gracefully");

    g_system_monitor->stop();
    // g_rest_server->stop();     // Disabled
    // g_netconf_server->stop();  // Disabled
    g_splitter_manager->shutdown();

    splitter::utils::Logger::info("Daemon shutdown complete");

  } catch (const std::exception &e) {
    splitter::utils::Logger::error("Fatal error: {}", e.what());
    return 1;
  }

  return 0;
}