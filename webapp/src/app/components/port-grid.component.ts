import { Component, Input, OnInit } from '@angular/core';
import { SystemStatus, PortStatus } from '../models/splitter.model';
import { SplitterService } from '../services/splitter.service';

@Component({
  selector: 'app-port-grid',
  templateUrl: './port-grid.component.html',
  styleUrls: ['./port-grid.component.scss']
})
export class PortGridComponent implements OnInit {
  @Input() systemStatus: SystemStatus | null = null;
  @Input() loading = false;
  
  selectedPort: PortStatus | null = null;
  
  constructor(private splitterService: SplitterService) {}
  
  ngOnInit() {}
  
  get ports(): PortStatus[] {
    return this.systemStatus?.ports || [];
  }
  
  onPortClick(port: PortStatus) {
    this.selectedPort = port;
  }
  
  togglePort(port: PortStatus) {
    if (this.loading) return;
    
    const action = port.state === 'enabled' 
      ? this.splitterService.disablePort(port.port_number)
      : this.splitterService.enablePort(port.port_number);
    
    action.subscribe({
      next: (result) => {
        if (!result.success) {
          console.error('Port operation failed:', result.message);
        }
      },
      error: (err) => {
        console.error('Port operation error:', err);
      }
    });
  }
  
  getPortColor(port: PortStatus): string {
    return this.splitterService.getPortStatusColor(port);
  }
  
  getPortIcon(port: PortStatus): string {
    return this.splitterService.getPortStatusIcon(port);
  }
  
  formatFrequency(frequencyMhz: number): string {
    return this.splitterService.formatFrequency(frequencyMhz);
  }
  
  formatPowerLevel(powerDbm: number): string {
    return this.splitterService.formatPowerLevel(powerDbm);
  }
  
  closePortDetails() {
    this.selectedPort = null;
  }
}