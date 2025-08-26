# Multi-stage Dockerfile for L-band Splitter C++ Backend
FROM ubuntu:22.04 AS build-stage

# Set build arguments
ARG BUILD_TYPE=Release
ARG CONAN_USER_HOME=/opt/conan

# Install system dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gcc-11 \
    g++-11 \
    python3 \
    python3-pip \
    pkg-config \
    git \
    curl \
    wget \
    libssl-dev \
    libsystemd-dev \
    libxml2-dev \
    libssh-dev \
    libcurl4-openssl-dev \
    libi2c-dev \
    libgpiod-dev \
    && rm -rf /var/lib/apt/lists/*

# Set compiler environment
ENV CC=gcc-11
ENV CXX=g++-11

# Install Conan
RUN pip3 install "conan>=2.0.0,<3.0.0"

# Create conan profile
RUN conan profile detect --force && \
    conan profile update settings.compiler.cppstd=17 default

# Copy source code
WORKDIR /app
COPY conanfile.txt CMakeLists.txt ./
COPY src/ src/
COPY config/ config/

# Install dependencies with Conan
RUN conan install . --build=missing --settings=build_type=${BUILD_TYPE}

# Build the application
RUN cmake --preset conan-release \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TESTING=OFF && \
    cmake --build --preset conan-release --parallel $(nproc)

# Runtime stage
FROM ubuntu:22.04 AS runtime-stage

# Install runtime dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libssl3 \
    libsystemd0 \
    libxml2 \
    libssh-4 \
    libcurl4 \
    libi2c-dev \
    libgpiod2 \
    ca-certificates \
    iproute2 \
    iputils-ping \
    procps \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Create application user
RUN groupadd -r splitter && \
    useradd -r -g splitter -s /bin/false -c "Splitter Daemon" splitter

# Create directories
RUN mkdir -p /etc/splitter /var/log/splitter /var/lib/splitter /usr/share/splitter/yang && \
    chown -R splitter:splitter /var/log/splitter /var/lib/splitter

# Copy built application
COPY --from=build-stage /app/build/Release/src/splitter_daemon /usr/local/bin/
COPY --from=build-stage /app/config/ /etc/splitter/

# Copy YANG models
COPY config/yang/ /usr/share/splitter/yang/

# Create default configuration
RUN echo '{ \
    "hardware": { \
        "gpio_base_pin": 100, \
        "status_led_base_pin": 200, \
        "signal_led_base_pin": 232, \
        "spi_device": "/dev/spidev0.0", \
        "i2c_device": "/dev/i2c-1", \
        "frequency_detector_address": 72 \
    }, \
    "network": { \
        "netconf_host": "0.0.0.0", \
        "netconf_port": 830, \
        "rest_host": "0.0.0.0", \
        "rest_port": 8080, \
        "enable_ssl": false \
    }, \
    "logging": { \
        "log_file": "/var/log/splitter/daemon.log", \
        "log_level": "info", \
        "max_file_size_mb": 100, \
        "max_files": 10 \
    }, \
    "monitoring": { \
        "enable_telegraf": true, \
        "telegraf_socket": "/tmp/telegraf.sock", \
        "metrics_interval_seconds": 30, \
        "enable_health_endpoint": true \
    } \
}' > /etc/splitter/config.json

# Create systemd service file
RUN echo '[Unit] \
Description=L-band Splitter/Combiner Daemon \
After=network.target \
Wants=network.target \
\n\
[Service] \
Type=notify \
ExecStart=/usr/local/bin/splitter_daemon /etc/splitter/config.json \
Restart=always \
RestartSec=5 \
User=splitter \
Group=splitter \
StandardOutput=journal \
StandardError=journal \
SyslogIdentifier=splitter \
WatchdogSec=30 \
\n\
[Install] \
WantedBy=multi-user.target' > /etc/systemd/system/splitter.service

# Set permissions
RUN chmod 644 /etc/systemd/system/splitter.service && \
    chmod 755 /usr/local/bin/splitter_daemon && \
    chmod 644 /etc/splitter/config.json

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Expose ports
EXPOSE 830/tcp 8080/tcp 8081/tcp

# Set working directory
WORKDIR /var/lib/splitter

# Switch to application user
USER splitter

# Default command
CMD ["/usr/local/bin/splitter_daemon", "/etc/splitter/config.json"]