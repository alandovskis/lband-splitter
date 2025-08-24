#!/usr/bin/env bash
# Build and launch the monitor daemon and simple web server
set -euo pipefail

# Build monitor_daemon if it does not exist
if [[ ! -x build/monitor_daemon ]]; then
  mkdir -p build
  conan profile detect --force >/dev/null 2>&1 || true
  conan install . --output-folder build --build=missing
  TOOLCHAIN_FILE=$(find build -name conan_toolchain.cmake | head -n 1)
  if [[ -z "$TOOLCHAIN_FILE" ]]; then
    echo "conan_toolchain.cmake not found; ensure 'conan install' completed successfully" >&2
    exit 1
  fi
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
  cmake --build build --target monitor_daemon
fi

DAEMON_BIN=$(find build -maxdepth 2 -type f -name monitor_daemon -perm -111 | head -n 1)
if [[ -z "$DAEMON_BIN" ]]; then
  echo "monitor_daemon binary not found in build directory" >&2
  exit 1
fi

"$DAEMON_BIN" &
DAEMON_PID=$!
echo "monitor_daemon running on port 9002 (PID $DAEMON_PID)"

if command -v npx >/dev/null 2>&1; then
  npx --yes http-server web -p 8080 &
  WEB_PID=$!
  echo "Web UI available at http://localhost:8080 (PID $WEB_PID)"
else
  echo "npx not found. Install Node.js and npm." >&2
  kill "$DAEMON_PID"
  exit 1
fi

trap "kill $DAEMON_PID $WEB_PID" EXIT

wait
