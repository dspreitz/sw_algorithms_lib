// Instruction count benchmark for WMM / WMMHR on Cortex-M4F in QEMU (-icount shift=0):
// the virtual clock advances 1 ns per instruction, CMSDK timer 0 counts that clock.
#include <stdio.h>
#include <stdint.h>
#include "wmmhr_eval.h"
#include "earth_induction_model.h"

static volatile uint32_t * const TIMER0 = (volatile uint32_t *)0x40000000; // CTRL, VALUE, RELOAD
static uint32_t now( void) { return TIMER0[1]; }

static const double POINTS[][2] = { { 52.14, 12.66}, { -44.49, 169.97}, { 38.95, -119.77}, { 80.0, -40.0}, { -23.87, 17.99} };
static const int NP = sizeof( POINTS) / sizeof( POINTS[0]);
volatile float sink;

int main( void)
{
  TIMER0[2] = 0xFFFFFFFF; TIMER0[1] = 0xFFFFFFFF; TIMER0[0] = 1;
  // calibration: 2 instructions per iteration
  uint32_t n = 1000000, t0 = now();
  __asm volatile ( "1: subs %0, %0, #1\n bne 1b" : "+r"( n));
  uint32_t ticks = t0 - now();
  double insn_per_tick = 2000000.0 / ticks;
  printf( "calibration: %lu ticks for 2e6 instructions -> %.2f instructions per tick\n", (unsigned long)ticks, insn_per_tick);

  struct { const char *name; int kind; } cases[] = {
      { "WMM2025 degree 12, PR #156 (float sum)", 0 },
      { "WMM degree 12, float (template)", 1 },
      { "WMMHR2025 degree 133, double", 2 },
      { "WMMHR2025 degree 133, float", 3 },
  };
  for( auto &c : cases)
    {
      uint32_t start = now();
      for( int i = 0; i < NP; ++i)
	{
	  double la = POINTS[i][0], lo = POINTS[i][1];
	  dec_inc r = { 0, 0};
	  switch( c.kind)
	    {
	    case 0: { induction_values v = earth_induction_model.get_induction_data_at( la, lo, 2026.75, 1.0); r.declination = v.declination; r.inclination = v.inclination; } break;
	    case 1: r = wmmhr<float>( la, lo, 2026.75, 1.0, 12); break;
	    case 2: r = wmmhr<double>( la, lo, 2026.75, 1.0, 133); break;
	    case 3: r = wmmhr<float>( la, lo, 2026.75, 1.0, 133); break;
	    }
	  sink = r.declination;
	  if( i < 3) printf( "  %s @ %.2f %.2f: decl %.3f incl %.3f\n", c.name, la, lo, r.declination, r.inclination);
	}
      double insn = ( start - now()) * insn_per_tick / NP;
      printf( "RESULT %s: %.0f instructions per call = %.2f ms at 168 MHz (1 instruction/cycle)\n", c.name, insn, insn / 168e3);
    }
  return 0;
}
