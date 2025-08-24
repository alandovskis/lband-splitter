import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';

export interface StatusResponse {
  enabled: boolean;
  frequencyHz: number;
}

@Injectable({ providedIn: 'root' })
export class StatusService {
  constructor(private http: HttpClient) {}

  getStatus(): Observable<StatusResponse> {
    return this.http.get<StatusResponse>('/api/status');
  }
}
