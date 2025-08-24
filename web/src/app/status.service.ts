import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';

export interface PortStatus {
  port: number;
  enabled: boolean;
  frequencyHz: number;
}

@Injectable({ providedIn: 'root' })
export class StatusService {
  private socket: WebSocketSubject<PortStatus[]>;

  constructor() {
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    this.socket = webSocket<PortStatus[]>(`${protocol}//${location.host}/api/status`);
  }

  getStatuses(): Observable<PortStatus[]> {
    return this.socket.asObservable();
  }
}
