// Simple PCM RIFF WAVE support

#ifndef _RIFF_
#define _RIFF_

#include <stdio.h>

#include "cdefs.h"

typedef void (*riff_error_t)( const char* msg );

void setup_riff_error( riff_error_t );

// „тение файла формата wav

class riffwave_reader {
public:
  // ѕараметр bufsize задает размер буфера дл€ варианта буфериованного чтени€ данных.
  // ≈сли значение этого параметра равно 0 (по умолчанию), то все данные читаютс€ сразу
  // и размер буфера совпадает с количеством отсчетов в файле (см. data_size()).
  riffwave_reader( const char* fname, int bufsize = 0 );

  ~riffwave_reader();

  int channels() const { return _channels; }
    // ѕолучение количества канало (моно/стерео)
  int samplespersec() const { return _samplespersec; }
    // ѕолучение количества отсчетов в секунду
  int bitspersample() const { return _bitspersample; }
    // ѕолучение количества битов на один отсчет

  int data_size() const { return _datasize / _alignment; }
    // ѕолучение количества отсчетов

  //  онстанты дл€ левого и правого каналов
  // ƒл€ монофонической записи считаетс€, что присутствует только левый канал
  enum channel { LEFT, RIGHT };

  // ѕолучение одного отсчета из заданного канала
  // √раницы изменени€ номера отсчета j определ€ютс€
  // размером буфера, заданным при конструировании.
  int operator()( int j, channel ch = LEFT ) const;

  // ѕозиционирование буфера на заданное место в файле
  void seek( int pos );
  // ѕолучение текущей позиции буфера
  int tell() const { return _bufpos; }
  // ѕолучение размера буфера
  int buf_size() const { return _buflen; }

private:
  FILE* _file;

  int _channels;
  int _samplespersec; // в одном канале
  int _bitspersample; // в одном канале
  int _alignment;     // байтов на один отсчет обоих каналов
  int _datasize;      // всего данных в байтах
  int _datapos;       // позици€ начала данных в файле в байтах

  int _bufpos;           // текуща€ позици€ буфера в отсчетах
  int _buflen;           // размер заполненной части буфера
  int _bufsize;          // полный размер буфера в байтах
  uchar* _data;

  int wait4data();
  int wait4fmt();
};

// ¬ывод массива data длиной len в одноканальный 16-битный wav с именем fname.
// »сходные данные должны представл€ть собой вещественные числа из диапазона [-1.0, 1.0].
// sps - частота дискретизации в герцах (количество отсчетов в секунду).
void save_as_riff( const char* fname, const double* data, int len, int sps );

#endif // _RIFF_
