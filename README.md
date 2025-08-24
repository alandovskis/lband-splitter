# L-Band Splitter/Combiner Control System

A comprehensive 32-port L-band (1-2 GHz) splitter/combiner system with real-time monitoring, web-based control interface, and NetConf management capabilities.

## Features

- **32 Independent Ports**: Each with enable/disable control and signal detection
- **Real-time Monitoring**: Frequency detection and power level measurement
- **Dual Control Interfaces**: Web-based GUI and NetConf protocol
- **Hardware Integration**: GPIO control for LEDs and I2C/SPI for sensors
- **Linux Service**: Systemd integration with proper security isolation

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Angular Web   │    │   NetConf API    │    │  Hardware Layer │
│   Interface     │◄──►│   (YANG Model)   │◄──►│   (GPIO/I2C)    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         ▲                       ▲                       ▲
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                    C++ Core System                               │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────────┐│
│  │ REST Server │ │NetConf Mgmt │ │    Splitter System Core     ││
│  │ (Port 8080) │ │ (Port 830)  │ │  - 32 Port Management       ││
│  └─────────────┘ └─────────────┘ │  - Frequency Detection       ││
│                                  │  - LED Control               ││
│                                  │  - Real-time Monitoring      ││
│                                  └─────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

## Quick Start

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install cmake g++ pkg-config npm git
sudo apt install libyang-dev libnetconf2-dev libjsoncpp-dev
sudo apt install libgtest-dev  # Optional, for tests
```

**RHEL/CentOS/Fedora:**
```bash
sudo dnf install cmake gcc-c++ pkgconfig npm git
sudo dnf install libyang-devel libnetconf2-devel jsoncpp-devel
sudo dnf install gtest-devel  # Optional, for tests
```

### Installation

1. **Clone and Build:**
   ```bash
   git clone <repository-url>
   cd splitter
   sudo ./scripts/install.sh
   ```

2. **Start Service:**
   ```bash
   sudo systemctl start splitter
   sudo systemctl enable splitter  # Auto-start on boot
   ```

3. **Access Interfaces:**
   - Web Interface: http://localhost:8080
   - NetConf: localhost:830

### Manual Build (Development)

```bash
# Build C++ application
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Build Angular webapp
cd ../webapp
npm install
npm run build

# Run in development mode
sudo ../build/splitter --config ../config/splitter.conf --debug
```

## Usage

### Web Interface

The Angular web application provides:

- **Port Grid View**: Visual overview of all 32 ports with LED status indicators
- **Port List View**: Detailed tabular view with sorting and filtering
- **System Overview**: Real-time charts and system health monitoring  
- **Configuration**: Port settings and system parameters

### NetConf Interface

Standards-compliant YANG model for programmatic control:

```bash
# Connect with netconf-console (example)
netconf-console --host=localhost --port=830 --user=admin --password=admin123

# Enable port 0
<rpc>
  <enable-port>
    <port-number>0</port-number>
  </enable-port>
</rpc>

# Get system status
<rpc>
  <get>
    <filter>
      <splitter-system/>
    </filter>
  </get>
</rpc>
```

### REST API

RESTful API for integration:

```bash
# Get system status
curl http://localhost:8080/api/v1/system/status

# Enable port 5
curl -X POST http://localhost:8080/api/v1/ports/5/control \
     -H "Content-Type: application/json" \
     -d '{"action": "enable"}'

# Get all port statuses
curl http://localhost:8080/api/v1/ports
```

## Hardware Requirements

### GPIO Pins

- **Enable LEDs**: GPIO 100-131 (ports 0-31)
- **Signal LEDs**: GPIO 132-163 (ports 0-31)

### I2C/SPI Devices

- **ADC**: ADS1115 for frequency detection (I2C address 0x48)
- **Displays**: SSD1306 OLED displays for frequency readouts
- **Temperature**: Optional system temperature monitoring

### Permissions

The service user needs access to:
```bash
# Add user to hardware access groups
sudo usermod -a -G gpio,i2c,spi splitter
```

## Configuration

### System Configuration

Edit `/etc/splitter/splitter.conf`:

```ini
[system]
name = "L-Band Splitter/Combiner"
num_ports = 32

[frequency_detection]
update_interval = 100         # milliseconds
signal_threshold = -80.0      # dBm

[web]
bind_address = "0.0.0.0"
port = 8080

[netconf]
bind_address = "0.0.0.0"
port = 830
```

### Port Configuration

Individual ports can be configured via:
- Web interface: Settings tab
- NetConf: YANG model configuration
- REST API: PUT `/api/v1/ports/{id}/config`

## Development

### Project Structure

```
├── src/
│   ├── hardware/          # GPIO and sensor abstraction
│   ├── core/             # Port management and system logic
│   ├── netconf/          # NetConf server implementation
│   ├── web/              # REST API server
│   └── main.cpp          # Service entry point
├── webapp/               # Angular frontend application
├── config/
│   ├── yang/            # YANG data models
│   └── splitter.conf    # System configuration
├── scripts/
│   ├── systemd/         # Service files
│   └── install.sh       # Installation script
└── tests/               # Unit and integration tests
```

### Building and Testing

```bash
# Debug build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests (if Google Test is available)
make test

# Code formatting
make format

# Static analysis
make analyze
```

### Adding New Features

1. **Hardware Integration**: Extend `src/hardware/` classes
2. **Port Logic**: Modify `src/core/splitter_port.cpp`
3. **API Endpoints**: Add routes in `src/web/rest_server.cpp`
4. **YANG Model**: Update `config/yang/splitter-system.yang`
5. **Frontend**: Add Angular components in `webapp/src/app/`

## Service Management

```bash
# Service control
sudo systemctl start splitter    # Start service
sudo systemctl stop splitter     # Stop service  
sudo systemctl restart splitter  # Restart service
sudo systemctl status splitter   # Check status

# Logs
sudo journalctl -u splitter -f   # Follow logs
sudo journalctl -u splitter --since="1 hour ago"

# Configuration reload
sudo systemctl reload splitter

# Enable/disable auto-start
sudo systemctl enable splitter
sudo systemctl disable splitter
```

## Troubleshooting

### Common Issues

1. **GPIO Permission Denied**
   ```bash
   sudo usermod -a -G gpio splitter
   sudo systemctl restart splitter
   ```

2. **Port Binding Error**
   ```bash
   sudo netstat -tulpn | grep :8080  # Check if port is in use
   sudo systemctl stop nginx         # Stop conflicting services
   ```

3. **NetConf Connection Failed**
   ```bash
   sudo ufw allow 830/tcp           # Open firewall port
   systemctl status splitter        # Check service status
   ```

### Debug Mode

```bash
# Run in foreground with debug output
sudo systemctl stop splitter
sudo /usr/local/bin/splitter --debug --config /etc/splitter/splitter.conf
```

### Log Files

- **System logs**: `journalctl -u splitter`
- **Application logs**: `/var/log/splitter/splitter.log`
- **Web server logs**: Embedded in application logs

## Security Considerations

- Service runs as dedicated `splitter` user with minimal privileges
- Network access restricted to configured interfaces
- Hardware access limited to required GPIO/I2C/SPI devices
- Web interface should be used behind reverse proxy in production
- NetConf authentication should use SSH keys in production

## License

[Your License Here]

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes and add tests
4. Run `make format` and `make test`
5. Submit a pull request

## Support

For issues and questions:
- Check logs: `sudo journalctl -u splitter -f`
- Review configuration: `/etc/splitter/splitter.conf`
- Web interface: http://localhost:8080
- GitHub Issues: [Repository Issues Page]