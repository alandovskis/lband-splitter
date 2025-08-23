import { Component, OnInit } from '@angular/core';

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
  private socket?: WebSocket;

  ngOnInit() {
    // Connect via secure WebSocket through the Nginx reverse proxy. The
    // proxy terminates TLS and forwards `/ws` traffic to the daemon.
    this.socket = new WebSocket(`wss://${location.host}/ws`);
    this.socket.onmessage = evt => {
      this.ports = JSON.parse(evt.data);
    };
  }

  toggle(idx: number) {
    const en = !this.ports[idx].enabled;
    this.socket?.send(JSON.stringify({ port: idx, enable: en }));
  }
}
