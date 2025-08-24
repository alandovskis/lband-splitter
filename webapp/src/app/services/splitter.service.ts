import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Observable, BehaviorSubject } from 'rxjs';
import { map, catchError } from 'rxjs/operators';
import { 
  SystemStatus, 
  SystemInfo, 
  PortStatus, 
  PortConfiguration,
  SystemConfiguration,
  OperationResult,
  PortControlRequest,
  SystemControlRequest
} from '../models/splitter.model';

@Injectable({
  providedIn: 'root'
})
export class SplitterService {
  private apiUrl = '/api/v1';
  private httpOptions = {
    headers: new HttpHeaders({
      'Content-Type': 'application/json'
    })
  };
  
  // Real-time status subject
  private systemStatusSubject = new BehaviorSubject<SystemStatus | null>(null);
  public systemStatus$ = this.systemStatusSubject.asObservable();
  
  constructor(private http: HttpClient) {}
  
  // System endpoints
  getSystemStatus(): Observable<SystemStatus> {
    return this.http.get<SystemStatus>(`${this.apiUrl}/system/status`)
      .pipe(
        map(status => {
          this.systemStatusSubject.next(status);
          return status;
        })
      );
  }
  
  getSystemInfo(): Observable<SystemInfo> {
    return this.http.get<SystemInfo>(`${this.apiUrl}/system/info`);
  }
  
  enableAllPorts(): Observable<OperationResult> {
    const request: SystemControlRequest = { action: 'enable_all' };
    return this.http.post<OperationResult>(`${this.apiUrl}/system/control`, request, this.httpOptions);
  }
  
  disableAllPorts(): Observable<OperationResult> {
    const request: SystemControlRequest = { action: 'disable_all' };
    return this.http.post<OperationResult>(`${this.apiUrl}/system/control`, request, this.httpOptions);
  }
  
  runSystemTest(): Observable<OperationResult> {
    return this.http.post<OperationResult>(`${this.apiUrl}/system/test`, {}, this.httpOptions);
  }
  
  calibrateSystem(): Observable<OperationResult> {
    return this.http.post<OperationResult>(`${this.apiUrl}/system/calibrate`, {}, this.httpOptions);
  }
  
  // Port endpoints
  getAllPorts(): Observable<PortStatus[]> {
    return this.http.get<PortStatus[]>(`${this.apiUrl}/ports`);
  }
  
  getPort(portNumber: number): Observable<PortStatus> {
    return this.http.get<PortStatus>(`${this.apiUrl}/ports/${portNumber}`);
  }
  
  enablePort(portNumber: number): Observable<OperationResult> {
    const request: PortControlRequest = { action: 'enable' };
    return this.http.post<OperationResult>(`${this.apiUrl}/ports/${portNumber}/control`, request, this.httpOptions);
  }
  
  disablePort(portNumber: number): Observable<OperationResult> {
    const request: PortControlRequest = { action: 'disable' };
    return this.http.post<OperationResult>(`${this.apiUrl}/ports/${portNumber}/control`, request, this.httpOptions);
  }
  
  getPortConfiguration(portNumber: number): Observable<{port: number, config: PortConfiguration}> {
    return this.http.get<{port: number, config: PortConfiguration}>(`${this.apiUrl}/ports/${portNumber}/config`);
  }
  
  setPortConfiguration(portNumber: number, config: PortConfiguration): Observable<OperationResult> {
    const request = { config };
    return this.http.put<OperationResult>(`${this.apiUrl}/ports/${portNumber}/config`, request, this.httpOptions);
  }
  
  // Utility methods
  isPortEnabled(port: PortStatus): boolean {
    return port.state === 'enabled';
  }
  
  hasSignal(port: PortStatus): boolean {
    return port.signal_detected;
  }
  
  formatFrequency(frequencyMhz: number): string {
    if (frequencyMhz === 0) {
      return 'N/A';
    }
    
    if (frequencyMhz >= 1000) {
      return `${(frequencyMhz / 1000).toFixed(3)} GHz`;
    } else {
      return `${frequencyMhz.toFixed(1)} MHz`;
    }
  }
  
  formatPowerLevel(powerDbm: number): string {
    return `${powerDbm.toFixed(1)} dBm`;
  }
  
  getPortStatusColor(port: PortStatus): string {
    switch (port.state) {
      case 'enabled':
        return port.signal_detected ? 'accent' : 'primary';
      case 'disabled':
        return 'basic';
      case 'error':
        return 'warn';
      default:
        return 'basic';
    }
  }
  
  getPortStatusIcon(port: PortStatus): string {
    switch (port.state) {
      case 'enabled':
        return port.signal_detected ? 'radio' : 'power';
      case 'disabled':
        return 'power_off';
      case 'error':
        return 'error';
      default:
        return 'help';
    }
  }
  
  // Real-time updates
  startRealTimeUpdates(intervalMs: number = 2000): void {
    setInterval(() => {
      this.getSystemStatus().subscribe(); // Updates the subject automatically
    }, intervalMs);
  }
  
  // Error handling
  private handleError<T>(operation = 'operation', result?: T) {
    return (error: any): Observable<T> => {
      console.error(`${operation} failed:`, error);
      throw error;
    };
  }
}