# Data Model: L-Band RF Splitter/Combiner

## Core Entities

### RFPort
**Purpose**: Represents each physical RF connector with its measurements and status

**Fields**:
- `id`: Integer (1-34) - Unique port identifier
- `type`: Enum - PORT_TYPE_F_COMBINER, PORT_TYPE_F_SPLITTER, PORT_TYPE_SMA_COMBINER_OUT, PORT_TYPE_SMA_SPLITTER_IN
- `status`: Enum - ACTIVE, DISABLED, ERROR, NO_SIGNAL  
- `center_frequency_mhz`: Float - Detected center frequency (950-2450 MHz)
- `power_level_dbm`: Float - Measured power level in dBm
- `snr_db`: Float - Signal-to-noise ratio in dB
- `modcod`: String - Detected modulation and coding (e.g., "QPSK-3/4", "8PSK-5/6")
- `signal_present`: Boolean - Signal detection status
- `last_updated`: Timestamp - Last measurement timestamp

**Validation Rules** (from FR-013, FR-014):
- center_frequency_mhz: 950 ≤ value ≤ 2450
- power_level_dbm: -80 ≤ value ≤ +20 (typical range)
- snr_db: 0 ≤ value ≤ 50 (typical range)

**State Transitions**:
```
NO_SIGNAL → ACTIVE (when signal detected)
ACTIVE → NO_SIGNAL (when signal lost)  
ACTIVE → ERROR (when signal outside parameters)
ERROR → DISABLED (when operator disables port)
DISABLED → ACTIVE (when operator enables port)
```

### PowerSupply
**Purpose**: Represents primary and secondary power sources with redundancy status

**Fields**:
- `id`: Integer (1-2) - PSU identifier  
- `status`: Enum - ACTIVE, STANDBY, FAILED, UNKNOWN
- `voltage`: Float - Output voltage in volts
- `current`: Float - Output current in amperes
- `temperature`: Float - PSU temperature in Celsius
- `is_primary`: Boolean - Primary/secondary designation
- `last_updated`: Timestamp

**Validation Rules** (from FR-010, FR-011):
- voltage: 11.0 ≤ value ≤ 13.0 (12V ±8%)
- current: 0.0 ≤ value ≤ 10.0 (max expected current)
- temperature: -10 ≤ value ≤ 70 (operating range)

**State Transitions**:
```
ACTIVE → FAILED (fault detected)
STANDBY → ACTIVE (takeover on primary failure)
FAILED → STANDBY (after repair/replacement)
```

### SystemConfiguration  
**Purpose**: Stores persistent system settings and operational parameters

**Fields**:
- `device_name`: String - System identifier
- `ip_address`: String - Network IP address
- `netmask`: String - Network subnet mask  
- `gateway`: String - Default gateway
- `ntp_server`: String - Time synchronization server
- `ssh_enabled`: Boolean - SSH access control
- `web_enabled`: Boolean - Web interface control
- `netconf_enabled`: Boolean - Netconf protocol control
- `measurement_interval_ms`: Integer - Data collection frequency (default: 16ms for 60Hz)
- `alarm_thresholds`: Object - Configurable alarm limits
- `last_modified`: Timestamp
- `config_version`: String - Configuration version number

**Validation Rules** (from FR-015, FR-016):
- measurement_interval_ms: 10 ≤ value ≤ 10000 (0.1Hz to 100Hz)
- IP addresses: Valid IPv4 format
- SSH/web/netconf: At least one interface must be enabled

### SignalMeasurement
**Purpose**: Time-series measurement data for trending and analysis

**Fields**:
- `port_id`: Integer - Reference to RFPort.id
- `timestamp`: Timestamp - Measurement time
- `center_frequency_mhz`: Float
- `power_level_dbm`: Float  
- `snr_db`: Float
- `modcod`: String
- `signal_quality`: Float - Derived quality metric (0-100)

**Relationships**: 
- Belongs to RFPort (foreign key: port_id)
- Retention: Local buffering (1 hour), external storage (1 year per FR-017)

### ControlSession
**Purpose**: Active management connections for audit and access control

**Fields**:
- `session_id`: String - Unique session identifier
- `interface_type`: Enum - CLI, WEB, NETCONF
- `user_id`: String - Authenticated user (for web/netconf)
- `remote_ip`: String - Client IP address
- `start_time`: Timestamp - Session start
- `last_activity`: Timestamp - Last command/request
- `commands_executed`: Integer - Command counter
- `is_active`: Boolean - Session status

**Validation Rules** (from FR-007, FR-008, FR-009):
- session_id: UUID format
- Interface access controlled by system configuration
- Session timeout: 30 minutes inactivity

## Data Relationships

```
SystemConfiguration (1) ←→ (1) System
System (1) ←→ (34) RFPort  
System (1) ←→ (2) PowerSupply
RFPort (1) ←→ (*) SignalMeasurement
System (1) ←→ (*) ControlSession
```

## Storage Strategy

**Configuration Data**: JSON files in `/etc/rf-splitter/` (persistent, replicated per FR-017)
**Measurement Data**: Ring buffers in memory (1 hour), external push via REST API  
**Session Data**: In-memory only (security)
**Logs**: Structured JSON logs to `/var/log/rf-splitter/`

## Data Flow Architecture

```
STM32F4 MCU → SPI/UART → C++ Backend → WebSocket/REST → Angular Frontend
              ↓
        JSON Config Files ← Config Manager → External Backup System
              ↓  
        Ring Buffer → External Data Store (1 year retention)
```