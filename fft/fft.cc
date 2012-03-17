#include "fft.h"
#include "cmplx.h"
#include "mdefs.h"
#include "basex.h"
#include "bits.h"
#include "trig_gen.h"

/*#lake:stop*/

namespace lwml {

cmplx fft::simple_dft( const vector& xr, const vector& xi, real n )
{
  test_size(xr.len(), xi.len());

  real sr = 0.0;
  real si = 0.0;
  int len = xr.len();
  trig_gen tg(2.0 * M_PI * n / len);
  for( int k = 0; k < len; k++ ){
    real sn = tg.get_sin();
    real cs = tg.get_cos();
    sr += xr[k] * cs + xi[k] * sn;
    si += xi[k] * cs - xr[k] * sn;
    tg.next();
  }
  return cmplx(sr/len, si/len);
}

cmplx fft::simple_idft( const vector& XR, const vector& XI, real k )
{
  test_size(XR.len(), XI.len());

  real sr = 0.0;
  real si = 0.0;
  int len = XR.len();
  trig_gen tg(2.0 * M_PI * k / len);
  for( int n = 0; n < len; n++ ){
    real sn = tg.get_sin();
    real cs = tg.get_cos();
    sr += XR[n] * cs - XI[n] * sn;
    si += XI[n] * cs + XR[n] * sn;
    tg.next();
  }
  return cmplx(sr, si);
}

// (1,-1),(-1,1),(1,0),(0,1) -> (1,1),(0,0),(3,-3),(0,-2)

void fft::binrevers( vector& x )
{
  int i, j, k;

  int n = x.len();
  for( j = i = 0; i < n - 1; i++ ){
    if( i < j )
      x.swap(i, j);
    for( k = n / 2; k <= j; k /= 2 )
      j -= k;
    j += k;
  }
}

void fft::base_fft( vector& x, vector& y )
{
  real ur, ui, wr, wi, tr, ti, ur2;
  int i, j, l, le1, le2, ip;

  test_size(x.len(), y.len());
  int n = x.len();
  if( !bits::is_pow2(n) )
    runtime("non-pow2 vector size <sz=%d>", n);
  int r = intlog2(n);

  binrevers(x);
  binrevers(y);
  for( le2 = l = 1; l <= r; l++ ){
    le1 = le2;
    le2 *= 2;
    ur = 1.0;
    ui = 0.0;
    wr = cos(M_PI / le1);
    wi = -sin(M_PI / le1);
    for( j = 0; j < le1; j++ ){
      for( i = j; i < n; i += le2 ){
        ip = i + le1;
        tr = x.at(ip) * ur - y.at(ip) * ui;
        ti = x.at(ip) * ui + y.at(ip) * ur;
        x.at(ip) = x.at(i);
        y.at(ip) = y.at(i);
        x.at(i) += tr;
        y.at(i) += ti;
        x.at(ip) -= tr;
        y.at(ip) -= ti;
      }
      ur2 = ur * wr - ui * wi;
      ui = ur * wi + ui * wr;
      ur = ur2;
    }
  }
}

void fft::scale( vector& a )
{
  int n = a.len();
  for( int i = 0; i < n; i++ )
    a.at(i) /= n;
}

void fft::resort( vector& a )
{
  int n = a.len();
  for( int i = 1; i < n / 2; i++ )
    a.swap(i, n-i);
}

void fft::cfft( vector& a, vector& b )
{
  scale(a);
  scale(b);
  base_fft(a, b);
}

void fft::cifft( vector& a, vector& b )
{
  resort(a);
  resort(b);
  base_fft(a, b);
}

void fft::disturb( vector& v )
{
  for( int j = 0; j < v.len(); j++ )
    if( j % 2 == 1 )
      v.at(j) = -v.at(j);
}

}; // namespace lwml
