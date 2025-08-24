export interface PortStatus {
  port_number: number;
  state: 'disabled' | 'enabled' | 'error';
  signal_detected: boolean;
  center_frequency_mhz: number;
  power_level_dbm: number;
  error_message: string;
  last_update: string;
}

export interface SystemStatus {
  enabled_ports: number;
  active_ports: number;
  system_healthy: boolean;
  system_error: string;
  last_update: string;
  ports: PortStatus[];
}

export interface SystemInfo {
  system_name: string;
  hardware_version: string;
  software_version: string;
  num_ports: number;
  api_version: string;
  uptime_seconds: number;
}

export interface PortConfiguration {
  enabled: boolean;
  description: string;
  signal_threshold: number;
}

export interface SystemConfiguration {
  enable_all_ports: boolean;
  monitoring_interval: number;
  signal_threshold: number;
}

export interface OperationResult {
  success: boolean;
  message: string;
  port?: number;
}

export interface PortControlRequest {
  action: 'enable' | 'disable';
}

export interface SystemControlRequest {
  action: 'enable_all' | 'disable_all';
}

export interface FrequencyData {
  timestamp: string;
  frequency_mhz: number;
  power_level_dbm: number;
}

export interface PortStatistics {
  port_number: number;
  total_uptime: number;
  signal_detection_count: number;
  error_count: number;
  frequency_history: FrequencyData[];
}