#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "fft.h"

// Прямое вычисление ДПФ по формуле
// xr  -- массив вещественных частей вектора исходных данных
// xi  -- массив мнимых частей вектора исходных данных
// len -- длина вектора исходных данных
// n   -- номер вычисляемого отсчета спектра
// res_r -- указатель на переменную, в которую будет помещена вещественная часть отсчета спектра
// res_i -- указатель на переменную, в которую будет помещена мнимая часть отсчета спектра

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

// Прямое вычисление обратного ДПФ по формуле
// XR  -- массив вещественных частей спектра
// XI  -- массив мнимых частей спектра
// len -- длина спектра
// k   -- номер вычисляемого отсчета данных
// res_r -- указатель на переменную, в которую будет помещена вещественная часть элемента данных
// res_i -- указатель на переменную, в которую будет помещена мнимая часть элемента данных

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
  double d[LEN]; // исходные данные
  
  // генерируем случайные данные
  srand(time(NULL));
  int j;
  for( j = 0; j < LEN; j++ )
    d[j] = (double)rand() / RAND_MAX - 0.5;

  // заполняем массивы для БПФ
  double xr[LEN], xi[LEN];
  for( j = 0; j < LEN; j++ ){
    xr[j] = d[j];
    xi[j] = 0.0;
  }

  // выполняем БПФ
  cfft(xr, xi, LEN);

  // выводим суммарный показатель невеществености для тех отсчетов, 
  // которые должны быть вещественными
  printf("im_error=%e\n", fabs(xi[0]) + fabs(xi[LEN/2]));
  // выводим значение спектрального отсчета на частоте Найквиста
  // (для случайных данных это значение не обязано быть малым)
  printf("discr_error=%e\n", fabs(xr[LEN/2]));

  // заполняем массивы для ДПФ
  double xr2[LEN], xi2[LEN];
  for( j = 0; j < LEN; j++ ){
    xr2[j] = d[j];
    xi2[j] = 0.0;
  }

  // вычисляем отсчеты ДПФ с помощью прямолинейного алгоритма
  // и сравниваем с результатом вычисление БПФ
  double err = 0.0;
  for( j = 0; j < LEN; j++ ){
    double sp_r, sp_i;

    simple_dft(xr2, xi2, LEN, j, &sp_r, &sp_i);
    err += fabs(sp_r - xr[j]) + fabs(sp_i - xi[j]);
  }
  printf("fft_error=%e\n", err);

  // выполняем обратное БПФ
  cifft(xr, xi, LEN);

  // и сравниваем результат с исходными данными
  double derr = 0.0;
  for( j = 0; j < LEN; j++ ){
    derr += fabs(xr[j] - d[j]) + fabs(xi[j]);
  }
  printf("ifft_error=%e\n", derr);
}
