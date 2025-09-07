# Quickstart Guide: L-Band RF Splitter/Combiner

## System Overview
This quickstart validates the complete L-Band RF splitter/combiner system through the primary user scenarios from the specification.

## Prerequisites
- L-Band RF splitter/combiner hardware connected to network
- Test signal sources (L-Band signal generator, satellite signals)
- Test measurement equipment (spectrum analyzer or RF power meter)
- Web browser or API testing tool (curl, Postman)

## Test Scenario 1: RF Signal Combining (FR-001)

### Setup
1. Connect L-Band test signals to F-type combiner ports 1-3
2. Connect spectrum analyzer to SMA combiner output
3. Verify system power and network connectivity

### Steps
1. **Access web interface**
   ```bash
   curl http://<system-ip>:8080/api/v1/status
   ```
   Expected: HTTP 200 with system status

2. **Verify combiner ports detected signals**
   ```bash
   curl http://<system-ip>:8080/api/v1/ports | jq '.[] | select(.type=="f_combiner")'
   ```
   Expected: Ports 1-3 show `signal_present: true`, frequency/power measurements

3. **Check combined output**
   - Spectrum analyzer should show combined signal at SMA output
   - Verify signal integrity per FR-014 (insertion loss ≤ 3dB per port)

4. **Web dashboard validation**
   - Open browser to `http://<system-ip>:8080`
   - Verify real-time display of frequency, power, SNR, modcod for each port
   - Confirm 60Hz update rate

**Pass Criteria**: All connected combiner ports show valid measurements, combined output present at SMA connector

## Test Scenario 2: RF Signal Splitting (FR-002)

### Setup  
1. Connect L-Band test signal to SMA splitter input
2. Connect RF power meters to F-type splitter ports 1, 5, 10, 16
3. Keep combiner test setup from Scenario 1

### Steps
1. **Verify splitter input detected**
   ```bash
   curl http://<system-ip>:8080/api/v1/ports | jq '.[] | select(.type=="sma_splitter_in")'
   ```
   Expected: Input port shows signal detection and measurements

2. **Check split outputs**
   ```bash
   curl http://<system-ip>:8080/api/v1/ports | jq '.[] | select(.type=="f_splitter")'
   ```
   Expected: All 16 splitter ports show same signal characteristics as input

3. **Validate signal distribution**
   - Power meters on ports 1, 5, 10, 16 should show equal levels (±1dB per FR-014)
   - Total insertion loss ≤ 6dB per FR-014

**Pass Criteria**: Signal distributed to all 16 splitter outputs with proper signal integrity

## Test Scenario 3: Multi-Interface Control (FR-007, FR-008, FR-009)

### Command Line Interface
```bash
# Test CLI access
ssh operator@<system-ip>
rf-control-cli --status
rf-control-cli --port 5 --disable
rf-control-cli --port 5 --enable
```

### Web Interface
```bash
# Test web API port control
curl -X POST http://<system-ip>:8080/api/v1/ports/5 \
  -H "Content-Type: application/json" \
  -d '{"enabled": false}'

curl -X POST http://<system-ip>:8080/api/v1/ports/5 \
  -H "Content-Type: application/json" \  
  -d '{"enabled": true}'
```

### WebSocket Real-time Data
```javascript
// Test WebSocket connection
const ws = new WebSocket('ws://<system-ip>:8080/ws/measurements');
ws.send('{"type": "subscribe_ports", "data": {"port_ids": [1,2,3]}}');
// Verify 60Hz measurement updates received
```

**Pass Criteria**: All three interfaces (CLI, REST, WebSocket) function correctly

## Test Scenario 4: Power Supply Redundancy (FR-010, FR-011)

### Setup
1. Verify both power supplies connected and operational
2. Maintain active RF signals from previous scenarios

### Steps
1. **Check power status**
   ```bash
   curl http://<system-ip>:8080/api/v1/power
   ```
   Expected: Both PSUs show "active" or "active"/"standby" status

2. **Simulate primary power failure**
   - Disconnect primary power supply
   - System should continue operation without interruption

3. **Verify failover**
   ```bash
   curl http://<system-ip>:8080/api/v1/power
   ```
   Expected: Secondary PSU shows "active", primary shows "failed"

4. **Confirm RF operation continues**
   - All port measurements continue updating
   - No signal interruption on spectrum analyzer/power meters

**Pass Criteria**: Seamless failover with no RF signal interruption

## Test Scenario 5: Signal Quality Monitoring (FR-003-FR-006)

### Steps
1. **Baseline measurements**
   ```bash
   curl http://<system-ip>:8080/api/v1/ports/1
   ```
   Record: frequency, power, SNR, modcod values

2. **Introduce signal degradation**
   - Add attenuator to reduce signal level
   - Verify measurements reflect changes

3. **Test alarm thresholds**
   - Reduce signal below alarm threshold
   - Verify alarm generation via WebSocket or system events

4. **Historical data validation**
   ```bash
   curl "http://<system-ip>:8080/api/v1/measurements/1?from=2025-09-06T10:00:00Z&limit=100"
   ```
   Expected: Historical measurement data available

**Pass Criteria**: Accurate measurement of all RF parameters with proper alarm generation

## Test Scenario 6: Network Resilience (FR-012)

### Steps
1. **Establish WebSocket connection**
   - Connect via WebSocket for real-time updates
   - Verify continuous data stream

2. **Simulate network interruption**  
   - Disconnect network cable for 10 seconds
   - Reconnect network

3. **Verify system behavior**
   - System continues RF operation during network outage
   - WebSocket automatically reconnects
   - No measurement data loss

**Pass Criteria**: System operates independently of network connectivity

## Performance Validation

### Signal Integrity (FR-014)
Measure and verify:
- Insertion loss: ≤ 3dB per combiner port, ≤ 6dB total splitter
- Return loss: ≥ 15dB at all ports
- Port isolation: ≥ 20dB between F-type ports  
- Phase matching: ±5° between ports
- Amplitude balance: ±1dB between ports

### Response Time (Performance Goals)
- Web API response: < 100ms
- WebSocket update rate: 60Hz (16.67ms intervals)
- Port enable/disable: < 1 second

## Expected Results Summary
- ✅ RF combining: 16 F-type inputs → 1 SMA output
- ✅ RF splitting: 1 SMA input → 16 F-type outputs  
- ✅ Real-time monitoring: frequency, power, SNR, modcod per port
- ✅ Multi-interface control: CLI, web, WebSocket, netconf
- ✅ Power redundancy: Seamless PSU failover
- ✅ Network resilience: Operation continues during network outages
- ✅ Signal integrity: All RF parameters within specification limits

## Troubleshooting
- **No signal detected**: Check RF connections, verify signal levels in range
- **Web interface not accessible**: Verify network settings, check firewall
- **WebSocket connection fails**: Check authentication, verify port 8080 open
- **Power failover not working**: Verify PSU connections, check PSU status LEDs