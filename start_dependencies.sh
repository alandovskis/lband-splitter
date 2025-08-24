#!/usr/bin/env bash
# Build and launch the monitor daemon and simple web server
set -euo pipefail

# Build monitor_daemon if it does not exist
if [[ ! -x build/monitor_daemon ]]; then
  conan profile detect --force >/dev/null 2>&1 || true
  conan install . --output-folder build --build=missing
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
  cmake --build build --target monitor_daemon
fi

./build/monitor_daemon &
DAEMON_PID=$!
echo "monitor_daemon running on port 9002 (PID $DAEMON_PID)"

if command -v npx >/dev/null 2>&1; then
  npx --yes http-server web -p 8080 &
  WEB_PID=$!
  echo "Web UI available at http://localhost:8080 (PID $WEB_PID)"
else
  echo "npx not found. Install Node.js and npm." >&2
  kill $DAEMON_PID
  exit 1
fi

trap "kill $DAEMON_PID $WEB_PID" EXIT

wait
