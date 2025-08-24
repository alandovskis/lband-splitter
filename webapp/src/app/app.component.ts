import { Component, OnInit, OnDestroy } from '@angular/core';
import { SplitterService } from './services/splitter.service';
import { SystemStatus, PortStatus } from './models/splitter.model';
import { Subject } from 'rxjs';
import { takeUntil } from 'rxjs/operators';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit, OnDestroy {
  title = 'L-Band Splitter Control';
  
  systemStatus: SystemStatus | null = null;
  selectedTab = 0;
  loading = false;
  error: string | null = null;
  
  private destroy$ = new Subject<void>();
  
  constructor(private splitterService: SplitterService) {}
  
  ngOnInit() {
    this.loadSystemStatus();
    this.startPeriodicUpdates();
  }
  
  ngOnDestroy() {
    this.destroy$.next();
    this.destroy$.complete();
  }
  
  loadSystemStatus() {
    this.loading = true;
    this.error = null;
    
    this.splitterService.getSystemStatus()
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (status) => {
          this.systemStatus = status;
          this.loading = false;
        },
        error: (err) => {
          this.error = 'Failed to load system status: ' + err.message;
          this.loading = false;
        }
      });
  }
  
  startPeriodicUpdates() {
    // Update every 2 seconds
    setInterval(() => {
      if (!this.loading) {
        this.loadSystemStatus();
      }
    }, 2000);
  }
  
  onTabChange(index: number) {
    this.selectedTab = index;
  }
  
  enableAllPorts() {
    this.loading = true;
    this.splitterService.enableAllPorts()
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (result) => {
          if (result.success) {
            this.loadSystemStatus();
          } else {
            this.error = result.message;
            this.loading = false;
          }
        },
        error: (err) => {
          this.error = 'Failed to enable all ports: ' + err.message;
          this.loading = false;
        }
      });
  }
  
  disableAllPorts() {
    this.loading = true;
    this.splitterService.disableAllPorts()
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (result) => {
          if (result.success) {
            this.loadSystemStatus();
          } else {
            this.error = result.message;
            this.loading = false;
          }
        },
        error: (err) => {
          this.error = 'Failed to disable all ports: ' + err.message;
          this.loading = false;
        }
      });
  }
  
  runSystemTest() {
    this.loading = true;
    this.splitterService.runSystemTest()
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (result) => {
          this.loading = false;
          if (result.success) {
            this.error = null;
            // Show success message
          } else {
            this.error = 'System test failed: ' + result.message;
          }
        },
        error: (err) => {
          this.error = 'Failed to run system test: ' + err.message;
          this.loading = false;
        }
      });
  }
  
  calibrateSystem() {
    this.loading = true;
    this.splitterService.calibrateSystem()
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (result) => {
          this.loading = false;
          if (result.success) {
            this.error = null;
            // Show success message
          } else {
            this.error = 'Calibration failed: ' + result.message;
          }
        },
        error: (err) => {
          this.error = 'Failed to calibrate system: ' + err.message;
          this.loading = false;
        }
      });
  }
  
  clearError() {
    this.error = null;
  }
}