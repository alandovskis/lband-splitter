# WebSocket API Contract: L-Band RF Splitter/Combiner

## Connection Endpoint
```
ws://{host}:8080/ws/measurements
```

## Authentication
- Basic Auth via WebSocket headers (same credentials as REST API)
- Connection rejected with 401 for invalid credentials

## Message Format
All messages are JSON with the following structure:
```json
{
  "type": "message_type",
  "timestamp": "2025-09-06T10:30:00.000Z",
  "data": { /* message-specific data */ }
}
```

## Client → Server Messages

### Subscribe to Port Measurements
```json
{
  "type": "subscribe_ports",
  "data": {
    "port_ids": [1, 2, 3],  // Empty array = all ports
    "interval_ms": 16       // Optional, defaults to system config
  }
}
```

**Response**: `subscription_ack` or `error`

### Unsubscribe from Measurements  
```json
{
  "type": "unsubscribe_ports", 
  "data": {
    "port_ids": [1, 2, 3]   // Empty array = unsubscribe all
  }
}
```

**Response**: `unsubscription_ack`

### Subscribe to System Events
```json
{
  "type": "subscribe_events",
  "data": {
    "event_types": ["power", "alarms", "config"]  // Empty = all events
  }
}
```

**Response**: `subscription_ack` or `error`

### Port Control Command
```json
{
  "type": "control_port",
  "data": {
    "port_id": 5,
    "action": "enable" | "disable"
  }
}
```

**Response**: `control_ack` or `error`

### Ping
```json
{
  "type": "ping",
  "data": {}
}
```

**Response**: `pong`

## Server → Client Messages

### Port Measurements (Real-time Stream)
```json
{
  "type": "port_measurements",
  "timestamp": "2025-09-06T10:30:00.123Z",
  "data": [
    {
      "port_id": 1,
      "center_frequency_mhz": 1575.42,
      "power_level_dbm": -65.5,
      "snr_db": 45.2,
      "modcod": "QPSK-3/4",
      "signal_present": true,
      "signal_quality": 92.5
    },
    {
      "port_id": 2,
      "center_frequency_mhz": null,
      "power_level_dbm": -85.0,
      "snr_db": null,
      "modcod": null,
      "signal_present": false,
      "signal_quality": 0
    }
  ]
}
```

### System Events
```json
{
  "type": "system_event",
  "timestamp": "2025-09-06T10:30:00.456Z", 
  "data": {
    "event_type": "power" | "alarm" | "config",
    "severity": "info" | "warning" | "error" | "critical",
    "message": "Power supply 1 switched to backup",
    "details": {
      "power_supply_id": 1,
      "previous_status": "active", 
      "current_status": "failed"
    }
  }
}
```

### Alarm Notifications
```json
{
  "type": "alarm",
  "timestamp": "2025-09-06T10:30:00.789Z",
  "data": {
    "alarm_id": "ALM-001",
    "port_id": 5,
    "alarm_type": "signal_loss" | "power_out_of_range" | "snr_low",
    "severity": "warning" | "error" | "critical", 
    "message": "Signal power below threshold",
    "current_value": -78.5,
    "threshold": -70.0,
    "acknowledged": false
  }
}
```

### Subscription Acknowledgment
```json
{
  "type": "subscription_ack",
  "timestamp": "2025-09-06T10:30:00.000Z",
  "data": {
    "subscription_type": "ports" | "events",
    "active_subscriptions": ["ports", "events"]
  }
}
```

### Control Acknowledgment
```json
{
  "type": "control_ack",
  "timestamp": "2025-09-06T10:30:00.000Z",
  "data": {
    "port_id": 5,
    "action": "enable",
    "success": true,
    "new_status": "active"
  }
}
```

### Error Response
```json
{
  "type": "error",
  "timestamp": "2025-09-06T10:30:00.000Z",
  "data": {
    "error_code": "INVALID_PORT" | "ACCESS_DENIED" | "SYSTEM_ERROR",
    "message": "Port ID 99 does not exist",
    "request_type": "subscribe_ports"
  }
}
```

### Pong Response  
```json
{
  "type": "pong",
  "timestamp": "2025-09-06T10:30:00.000Z",
  "data": {}
}
```

## Connection Management

### Connection States
- `CONNECTING`: Initial connection attempt
- `CONNECTED`: Authenticated and ready
- `SUBSCRIBED`: Active subscriptions established  
- `ERROR`: Connection error state
- `DISCONNECTED`: Connection closed

### Reconnection Strategy
- Client implements exponential backoff: 1s, 2s, 4s, 8s, max 30s
- Server maintains subscription state for 60 seconds after disconnect
- Resume subscriptions automatically on reconnect

### Heartbeat
- Client sends `ping` every 30 seconds
- Server responds with `pong` within 5 seconds
- Connection considered dead if 3 consecutive pings fail

## Error Codes
- `INVALID_PORT`: Port ID out of range (1-34)
- `INVALID_MESSAGE`: Malformed JSON or unknown message type
- `ACCESS_DENIED`: Insufficient privileges for requested action
- `SYSTEM_ERROR`: Internal system error
- `RATE_LIMITED`: Too many requests (max 100/second)
- `SUBSCRIPTION_LIMIT`: Maximum subscriptions exceeded (50 per client)

## Performance Characteristics
- **Maximum Update Rate**: 100 Hz (10ms intervals)
- **Default Update Rate**: 60 Hz (16.67ms intervals)  
- **Maximum Concurrent Clients**: 10
- **Message Compression**: gzip supported
- **Maximum Message Size**: 64KB
- **Connection Timeout**: 60 seconds idle