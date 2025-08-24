import { TestBed } from '@angular/core/testing';
import { of } from 'rxjs';
import { DashboardComponent } from './dashboard.component';
import { StatusService } from './status.service';

describe('DashboardComponent', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      imports: [DashboardComponent],
      providers: [
        {
          provide: StatusService,
          useValue: {
            getStatus: () => of({ enabled: true, frequencyHz: 123_000_000 })
          }
        }
      ]
    });
  });

  it('should display enabled status and frequency in MHz', () => {
    const fixture = TestBed.createComponent(DashboardComponent);
    fixture.detectChanges();
    const compiled = fixture.nativeElement as HTMLElement;
    expect(compiled.textContent).toContain('Enabled: true');
    expect(compiled.textContent).toContain('Detected Frequency: 123 MHz');
  });
});
