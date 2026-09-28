// Evaluation of WMM(HR) up to a given degree, templated on the float type.
#include <math.h>
#include <stdint.h>
#include "wmmhr_coefficients.h"
struct dec_inc { float declination, inclination; };

template <typename T> static inline T tsqrt( T x) { return sqrt( x); }
template <> inline float tsqrt<float>( float x) { return sqrtf( x); }
template <typename T> static inline T tsin( T x) { return sin( x); }
template <> inline float tsin<float>( float x) { return sinf( x); }
template <typename T> static inline T tcos( T x) { return cos( x); }
template <> inline float tcos<float>( float x) { return cosf( x); }
template <typename T> static inline T tatan2( T y, T x) { return atan2( y, x); }
template <> inline float tatan2<float>( float y, float x) { return atan2f( y, x); }

template <typename T>
dec_inc wmmhr( double latitude, double longitude, double decimal_year, double altitude_km, unsigned degree)
{
  const T DEG = (T)( 3.14159265358979323846 / 180.0);
  const T A = (T)6378.137, F = (T)( 1.0 / 298.257223563), E2 = F * ( (T)2 - F);
  // geodetic -> geocentric (in double: cheap, done once)
  double sl = sin( latitude * 3.14159265358979323846 / 180.0), cl = cos( latitude * 3.14159265358979323846 / 180.0);
  double rc = 6378.137 / sqrt( 1.0 - (double)E2 * sl * sl);
  double p = ( rc + altitude_km) * cl, z = ( rc * ( 1.0 - (double)E2) + altitude_km) * sl;
  double r = sqrt( p * p + z * z);
  double gclat = asin( z / r);
  T x = (T)sin( gclat), s = (T)cos( gclat), recip_s = 1 / s;
  T years = (T)( decimal_year - WMMHR_EPOCH);
  T ratio = (T)( 6371.2 / r);
  T cl1 = (T)cos( longitude * 3.14159265358979323846 / 180.0), sl1 = (T)sin( longitude * 3.14159265358979323846 / 180.0);
  (void)A;

  T north = 0, east = 0, down = 0;
  T p_mm = 1, ratio_m2 = ratio * ratio; // ratio^(m+2)
  T cos_ml = 1, sin_ml = 0;
  for( unsigned m = 0; m <= degree; ++m)
    {
      if( m == 1) p_mm = s;
      else if( m > 1) p_mm *= tsqrt<T>( ( 2 * m - (T)1) / ( 2 * m)) * s;
      if( m > 0)
	{
	  T c = cos_ml * cl1 - sin_ml * sl1; sin_ml = sin_ml * cl1 + cos_ml * sl1; cos_ml = c;
	  ratio_m2 *= ratio;
	}
      if( p_mm == 0) break; // underflow near the poles: remaining terms are negligible
      T p_n1 = 0, p_n = p_mm, ratio_n2 = ratio_m2, root_n1 = 0;
      for( unsigned n = m; n <= degree; ++n)
	{
	  T root_n = tsqrt<T>( (T)( n * n - m * m));
	  if( n > m)
	    {
	      T nxt = ( ( 2 * (T)n - 1) * x * p_n - root_n1 * p_n1) / root_n;
	      p_n1 = p_n; p_n = nxt; ratio_n2 *= ratio;
	    }
	  root_n1 = root_n;
	  if( n == 0) continue;
	  T dp = ( n * x * p_n - root_n * p_n1) * recip_s;
	  unsigned idx = n * ( n + 1) / 2 + m - 1;
	  T g, h;
	  if( n <= WMMHR_CORE_DEGREE)
	    { const wmmhr_core_t &c = WMMHR_CORE[ idx]; g = c.g + years * c.dg; h = c.h + years * c.dh; }
	  else
	    { const wmm_crust_t &c = WMMHR_CRUST[ idx - 135]; g = c.g * (T)1e-4; h = c.h * (T)1e-4; }
	  T ghc = g * cos_ml + h * sin_ml;
	  north += ratio_n2 * ghc * dp;
	  east += ratio_n2 * m * ( g * sin_ml - h * cos_ml) * p_n;
	  down -= ( n + (T)1) * ratio_n2 * ghc * p_n;
	}
    }
  east *= recip_s;
  T psi = (T)( gclat - latitude * 3.14159265358979323846 / 180.0);
  T ng = north * tcos<T>( psi) - down * tsin<T>( psi);
  T dg = north * tsin<T>( psi) + down * tcos<T>( psi);
  dec_inc ret = { (float)( tatan2<T>( east, ng) / DEG), (float)( tatan2<T>( dg, tsqrt<T>( ng * ng + east * east)) / DEG) };
  return ret;
}
