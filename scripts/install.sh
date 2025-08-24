#!/bin/bash
# L-Band Splitter Installation Script

set -e  # Exit on any error

INSTALL_PREFIX="/usr/local"
CONFIG_DIR="/etc/splitter"
LOG_DIR="/var/log/splitter"
DATA_DIR="/var/lib/splitter"
WEB_DIR="/var/www/splitter"
YANG_DIR="${INSTALL_PREFIX}/share/yang/modules"

SERVICE_USER="splitter"
SERVICE_GROUP="splitter"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

check_dependencies() {
    print_status "Checking dependencies..."
    
    local deps=("cmake" "g++" "pkg-config" "npm")
    local missing_deps=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Install them with your package manager:"
        print_status "  Ubuntu/Debian: sudo apt install cmake g++ pkg-config npm libyang-dev libnetconf2-dev libjsoncpp-dev"
        print_status "  RHEL/CentOS: sudo yum install cmake gcc-c++ pkgconfig npm libyang-devel libnetconf2-devel jsoncpp-devel"
        exit 1
    fi
    
    print_status "All dependencies found"
}

create_user() {
    print_status "Creating service user..."
    
    if ! id "$SERVICE_USER" &>/dev/null; then
        useradd --system --shell /bin/false --home-dir /nonexistent --no-create-home "$SERVICE_USER"
        print_status "Created user: $SERVICE_USER"
    else
        print_status "User $SERVICE_USER already exists"
    fi
    
    # Add user to required groups for hardware access
    local groups=("gpio" "i2c" "spi")
    for group in "${groups[@]}"; do
        if getent group "$group" &>/dev/null; then
            usermod -a -G "$group" "$SERVICE_USER"
            print_status "Added $SERVICE_USER to group: $group"
        else
            print_warning "Group $group does not exist (hardware access may be limited)"
        fi
    done
}

create_directories() {
    print_status "Creating directories..."
    
    local dirs=("$CONFIG_DIR" "$LOG_DIR" "$DATA_DIR" "$WEB_DIR" "$YANG_DIR")
    
    for dir in "${dirs[@]}"; do
        mkdir -p "$dir"
        print_status "Created directory: $dir"
    done
    
    # Set ownership
    chown root:root "$CONFIG_DIR"
    chown "$SERVICE_USER:$SERVICE_GROUP" "$LOG_DIR" "$DATA_DIR"
    chown root:root "$WEB_DIR"
    chown root:root "$YANG_DIR"
    
    # Set permissions
    chmod 755 "$CONFIG_DIR" "$WEB_DIR" "$YANG_DIR"
    chmod 750 "$LOG_DIR" "$DATA_DIR"
}

build_application() {
    print_status "Building application..."
    
    if [[ ! -f "CMakeLists.txt" ]]; then
        print_error "CMakeLists.txt not found. Run this script from the project root directory."
        exit 1
    fi
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure and build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" ..
    make -j$(nproc)
    
    # Build webapp
    make webapp
    
    cd ..
    
    print_status "Build completed successfully"
}

install_application() {
    print_status "Installing application..."
    
    cd build
    make install
    cd ..
    
    # Install configuration files
    cp config/splitter.conf "$CONFIG_DIR/"
    chmod 644 "$CONFIG_DIR/splitter.conf"
    
    # Install YANG models
    cp config/yang/*.yang "$YANG_DIR/"
    chmod 644 "$YANG_DIR"/*.yang
    
    # Install webapp
    if [[ -d "webapp/dist" ]]; then
        cp -r webapp/dist/* "$WEB_DIR/"
        chown -R root:root "$WEB_DIR"
        chmod -R 644 "$WEB_DIR"
        find "$WEB_DIR" -type d -exec chmod 755 {} \;
    fi
    
    print_status "Application installed successfully"
}

install_service() {
    print_status "Installing systemd service..."
    
    cp scripts/systemd/splitter.service /etc/systemd/system/
    chmod 644 /etc/systemd/system/splitter.service
    
    systemctl daemon-reload
    systemctl enable splitter.service
    
    print_status "Systemd service installed and enabled"
}

configure_firewall() {
    print_status "Configuring firewall..."
    
    if command -v ufw &> /dev/null; then
        ufw allow 8080/tcp comment "L-Band Splitter Web Interface"
        ufw allow 830/tcp comment "L-Band Splitter NetConf"
        print_status "UFW rules added"
    elif command -v firewall-cmd &> /dev/null; then
        firewall-cmd --permanent --add-port=8080/tcp
        firewall-cmd --permanent --add-port=830/tcp
        firewall-cmd --reload
        print_status "Firewalld rules added"
    else
        print_warning "No firewall manager detected. Please manually open ports 8080 and 830"
    fi
}

create_logrotate_config() {
    print_status "Creating log rotation configuration..."
    
    cat > /etc/logrotate.d/splitter << EOF
$LOG_DIR/splitter.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    copytruncate
    notifempty
    su $SERVICE_USER $SERVICE_GROUP
}
EOF
    
    print_status "Log rotation configured"
}

print_summary() {
    print_status "Installation completed successfully!"
    echo
    echo "Service Management:"
    echo "  Start service:    sudo systemctl start splitter"
    echo "  Stop service:     sudo systemctl stop splitter"
    echo "  Service status:   sudo systemctl status splitter"
    echo "  View logs:        sudo journalctl -u splitter -f"
    echo
    echo "Web Interface:"
    echo "  URL: http://localhost:8080"
    echo
    echo "NetConf Interface:"
    echo "  Host: localhost"
    echo "  Port: 830"
    echo
    echo "Configuration:"
    echo "  Config file: $CONFIG_DIR/splitter.conf"
    echo "  Log files:   $LOG_DIR/"
    echo "  Data files:  $DATA_DIR/"
    echo
    print_status "To start the service now: sudo systemctl start splitter"
}

main() {
    print_status "Starting L-Band Splitter installation..."
    
    check_root
    check_dependencies
    create_user
    create_directories
    build_application
    install_application
    install_service
    configure_firewall
    create_logrotate_config
    print_summary
}

# Run main function
main "$@"