import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { StatusService, PortStatus } from './status.service';

@Component({
  selector: 'app-dashboard',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.css'
})
export class DashboardComponent implements OnInit {
  statuses?: PortStatus[];

  constructor(private status: StatusService) {}

  ngOnInit(): void {
    this.status.getStatuses().subscribe(res => {
      this.statuses = res;
    });
  }
}
