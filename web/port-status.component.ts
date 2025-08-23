import { Component, OnInit } from '@angular/core';
import { HttpClient } from '@angular/common/http';

interface Port {
  enabled: boolean;
  signalPresent: boolean;
  centerFreqMHz: number;
}

@Component({
  selector: 'app-port-status',
  templateUrl: './port-status.component.html'
})
export class PortStatusComponent implements OnInit {
  ports: Port[] = [];

  constructor(private http: HttpClient) {}

  ngOnInit() { this.refresh(); }

  refresh() {
    this.http.get<Port[]>('/api/ports').subscribe(data => this.ports = data);
  }

  toggle(idx: number) {
    const en = !this.ports[idx].enabled;
    this.http.post(`/api/ports/${idx}/enable`, { enabled: en })
        .subscribe(() => this.refresh());
  }
}
