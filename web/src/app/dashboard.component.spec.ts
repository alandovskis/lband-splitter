import { TestBed } from '@angular/core/testing';
import { NoopAnimationsModule } from '@angular/platform-browser/animations';
import { of } from 'rxjs';
import { DashboardComponent } from './dashboard.component';
import { StatusService, PortStatus } from './status.service';

describe('DashboardComponent', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      imports: [DashboardComponent, NoopAnimationsModule],
      providers: [
        {
          provide: StatusService,
          useValue: {
            getStatuses: () =>
              of(
                Array.from({ length: 32 }, (_, i) => ({
                  port: i,
                  enabled: i % 2 === 0,
                  frequencyHz: 123_000_000
                })) as PortStatus[]
              )
          }
        }
      ]
    });
  });

  it('should display a table of port statuses with frequency in MHz', () => {
    const fixture = TestBed.createComponent(DashboardComponent);
    fixture.detectChanges();
    const compiled = fixture.nativeElement as HTMLElement;
    expect(compiled.querySelectorAll('tr.mat-mdc-row').length).toBe(32);
    expect(compiled.textContent).toContain('Port');
    expect(compiled.textContent).toContain('123');
  });
});
