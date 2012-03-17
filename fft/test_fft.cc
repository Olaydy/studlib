#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "fft.h"

// ������ ���������� ��� �� �������
// xr  -- ������ ������������ ������ ������� �������� ������
// xi  -- ������ ������ ������ ������� �������� ������
// len -- ����� ������� �������� ������
// n   -- ����� ������������ ������� �������
// res_r -- ��������� �� ����������, � ������� ����� �������� ������������ ����� ������� �������
// res_i -- ��������� �� ����������, � ������� ����� �������� ������ ����� ������� �������

void simple_dft( double* xr, double* xi, int len, int n, double* res_r, double* res_i )
{
  double sr = 0.0;
  double si = 0.0;
  int k;

  for( k = 0; k < len; k++ ){
    double sn = sin(2.0 * M_PI * n * k / len);
    double cs = cos(2.0 * M_PI * n * k / len);
    sr += xr[k] * cs + xi[k] * sn;
    si += xi[k] * cs - xr[k] * sn;
  }
  *res_r = sr/len;
  *res_i = si/len;
}

// ������ ���������� ��������� ��� �� �������
// XR  -- ������ ������������ ������ �������
// XI  -- ������ ������ ������ �������
// len -- ����� �������
// k   -- ����� ������������ ������� ������
// res_r -- ��������� �� ����������, � ������� ����� �������� ������������ ����� �������� ������
// res_i -- ��������� �� ����������, � ������� ����� �������� ������ ����� �������� ������

void simple_idft( double* XR, double* XI, int len, int k, double* res_r, double* res_i )
{
  double sr = 0.0;
  double si = 0.0;
  int n;

  for( n = 0; n < len; n++ ){
    double sn = sin(2.0 * M_PI * n * k / len);
    double cs = cos(2.0 * M_PI * n * k / len);
    sr += XR[n] * cs - XI[n] * sn;
    si += XI[n] * cs + XR[n] * sn;
  }
  *res_r = sr;
  *res_i = si;
}

#define LEN 4096

int main()
{
  double d[LEN]; // �������� ������
  
  // ���������� ��������� ������
  srand(time(NULL));
  int j;
  for( j = 0; j < LEN; j++ )
    d[j] = (double)rand() / RAND_MAX - 0.5;

  // ��������� ������� ��� ���
  double xr[LEN], xi[LEN];
  for( j = 0; j < LEN; j++ ){
    xr[j] = d[j];
    xi[j] = 0.0;
  }

  // ��������� ���
  cfft(xr, xi, LEN);

  // ������� ��������� ���������� ��������������� ��� ��� ��������, 
  // ������� ������ ���� �������������
  printf("im_error=%e\n", fabs(xi[0]) + fabs(xi[LEN/2]));
  // ������� �������� ������������� ������� �� ������� ���������
  // (��� ��������� ������ ��� �������� �� ������� ���� �����)
  printf("discr_error=%e\n", fabs(xr[LEN/2]));

  // ��������� ������� ��� ���
  double xr2[LEN], xi2[LEN];
  for( j = 0; j < LEN; j++ ){
    xr2[j] = d[j];
    xi2[j] = 0.0;
  }

  // ��������� ������� ��� � ������� �������������� ���������
  // � ���������� � ����������� ���������� ���
  double err = 0.0;
  for( j = 0; j < LEN; j++ ){
    double sp_r, sp_i;

    simple_dft(xr2, xi2, LEN, j, &sp_r, &sp_i);
    err += fabs(sp_r - xr[j]) + fabs(sp_i - xi[j]);
  }
  printf("fft_error=%e\n", err);

  // ��������� �������� ���
  cifft(xr, xi, LEN);

  // � ���������� ��������� � ��������� �������
  double derr = 0.0;
  for( j = 0; j < LEN; j++ ){
    derr += fabs(xr[j] - d[j]) + fabs(xi[j]);
  }
  printf("ifft_error=%e\n", derr);
}
