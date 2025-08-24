import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { StatusService } from './status.service';

@Component({
  selector: 'app-dashboard',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.css'
})
export class DashboardComponent implements OnInit {
  enabled?: boolean;
  frequencyMHz?: number;

  constructor(private status: StatusService) {}

  ngOnInit(): void {
    this.status.getStatus().subscribe(res => {
      this.enabled = res.enabled;
      this.frequencyMHz = res.frequencyHz / 1_000_000;
    });
  }
}
