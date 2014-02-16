#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "riff.h"
#include "fft.h"
#include "bmp.h"

// Параметры спектрального анализа

const double win_len_sec = 0.250; // длина окна для спектрального анализа (sec)
const double win_shift_sec = 0.010; // сдвиг окна (sec)
const double max_freq_hz = 2000; // максимальная частота на спектрограмме (Hz)

int up2pow2( int x ) // округление до степени двойки
{
  int res = 1;
  while( res < x )
    res *= 2;
  return res;
}

double scale( double x, double a, double b, double c, double d ){
  return c + (d-c) * (x-a)/(b-a);
}

// спектральный анализ одного окна

double asymgannw( double n, double len )
{
  return (n < 0 || n > len) ? 0.0 : (1.0 - cos(2.0 * M_PI * n / len)) / 2.0;
}

class spectrum {
public:
  // длина должна быть степенью двойки
  spectrum( int win_len ) : _win_len(win_len) {
    _re = new double[_win_len];
    _im = new double[_win_len];
  }

  ~spectrum(){
    delete []_re;
    delete []_im;
  }

  void proc( double* pw_sp, double* data ){
    // считаем среднее
    double sum = 0.0;
    for( int j = 0; j < _win_len; j++)
      sum += data[j];
    double mid = sum / _win_len;

    // центрируем данные
    for( int j = 0; j < _win_len; j++){
      _re[j] = (data[j] - mid) * asymgannw(j, _win_len);
      _im[j] = 0.0;
    }

    cfft(_re, _im, _win_len);

    int sp_len = _win_len / 2 + 1;
    for( int j = 0; j < sp_len; j++ )
      pw_sp[j] = hypot(_re[j], _im[j]);
  }

private:
  int _win_len;
  double *_re, *_im;
};

double sp_func( double val )
{
  return val;
}

int main( int argc, char *argv[] ){
  if( argc != 2 ){
    printf("usage: riff_spgram wav-file-name\n");
    exit(1);
  }

  // создаем объект
  riffwave_reader in(argv[1]);

  // печатаем общую информацию о файле
  printf("channels=%d\n", in.channels());
  printf("bits_per_sample=%d\n", in.bitspersample());

  int data_len = in.data_size();
  int samples_per_sec = in.samplespersec();
  printf("data_len=%d\n", data_len);
  printf("samples_per_sec=%d\n", samples_per_sec);

  // считаем параметры спектрального анализа
  double sec_per_sample = 1.0 / samples_per_sec;
  int win_len_samples = win_len_sec / sec_per_sample;
  printf("win_len_samples=%d\n", win_len_samples);
  int win_len_samples_p2 = up2pow2(win_len_samples);
  printf("win_len_samples_p2=%d\n", win_len_samples_p2);

  int win_shift_samples = win_shift_sec / sec_per_sample;
  printf("win_shift_samples=%d\n", win_shift_samples);

  int pw_sp_len = win_len_samples_p2 / 2 + 1; // длина спектра мощности
  printf("pw_sp_len=%d\n", pw_sp_len);

  double hz_per_sample = 1.0 / (win_len_samples_p2 * sec_per_sample); // шаг по частоте в спектре
  int max_freq_samples = max_freq_hz / hz_per_sample;
  printf("max_freq_samples=%d\n", max_freq_samples);
  if( max_freq_samples > pw_sp_len - 1 ){
    printf("error: max_freq_hz is too large\n");
    exit(1);
  }

  // считаем количество окон (можно написать формулу, но так очевиднее)
  int win_num = 0;
  for( int first_sample = 0; first_sample < data_len; first_sample += win_shift_samples ){
    int last_sample = first_sample + (win_len_samples_p2 - 1);
    if( last_sample > data_len - 1 )
      break;
    ++win_num;
  }
  printf("win_num=%d\n", win_num);

  // собственно спектральный анализ последовательных окон
  double* data = new double[win_len_samples_p2];
  double* pw_sp = new double[pw_sp_len];
  double* long_pw_sp = new double[(max_freq_samples + 1) * win_num];

  spectrum sp(win_len_samples_p2);

  for( int j = 0; j < win_num; j++ ){
    if( j != 0 && j % 10 == 0 ){
      printf("%d ", j);
      fflush(stdout);
    }
    int first_sample = j * win_shift_samples;
    for( int k = 0; k < win_len_samples_p2; k++ )
      data[k] = in(first_sample + k, riffwave_reader::LEFT); // берем только левый канал

    sp.proc(pw_sp, data);

    int first_sample_sp = (max_freq_samples + 1) * j;
    for( int j = 0; j <= max_freq_samples; j++ )
      long_pw_sp[first_sample_sp + j] = pw_sp[j];
  }
  printf("\n");
  fflush(stdout);

  // находим максимальное значение мощности
  double min_sp_val = sp_func(long_pw_sp[0]);
  double max_sp_val = min_sp_val;
  for( int j = 0; j < (max_freq_samples + 1) * win_num; j++ ){
    double val = sp_func(long_pw_sp[j]);
    if( val < min_sp_val )
      min_sp_val = val;
    if( val > max_sp_val )
      max_sp_val = val;
  }
  printf("min_sp_val=%f\n", min_sp_val);
  printf("max_sp_val=%f\n", max_sp_val);
  fflush(stdout);

  // и отрисовываем спектр в картинке
  printf("drawing bitmap...\n");
  fflush(stdout);
  bitmap bmp(max_freq_samples + 1, win_num, bmp_pal());
  for( int y = 0; y < bmp.height(); y++ ){
    for( int x = 0; x < bmp.width(); x++ ){
      int idx = x * (max_freq_samples + 1) + y;
      int val = scale(sp_func(long_pw_sp[idx]), min_sp_val, max_sp_val, 0, 255);
      bmp.put(y, x, bmp_rgb(val, val, val));
    }
  }

  printf("saving bitmap...\n");
  fflush(stdout);
  bmp.save("spectrum.bmp");

  delete []long_pw_sp;
  delete []pw_sp;
  delete []data;
}
