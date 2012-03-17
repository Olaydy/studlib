// lwml, (c) ltwood

#ifndef _RIFF_
#define _RIFF_

// PCM WAVE RIFF
// Pulse Code Modulation
// Resource Interchange File Format

#include "defs.h"
#include "vector.h"
#include "stream.h"
#include "t_membuf.h"

/*#lake:stop*/

namespace lwml {

// Исключение при ошибке интерпретации содержимого RIFF-Файла

DEF_EX_TYPE_MSG(ex_integrity, ex_riff, "incorrect riff");

class riffwave_reader : public value {
public:
  // Параметр bufsize задает размер буфера для варианта буфериованного чтения данных.
  // Если значение этого параметра равно 0 (по умолчанию), то все данные читаются сразу.
  riffwave_reader( const char*, int bufsize = 0 );

  void printinfo( referer<stream> ) const;

  int channels() const { return _channels; }
  int samplespersec() const { return _samplespersec; }
  int bitspersample() const { return _bitspersample; }

  int data_size() const { return _datasize / _alignment; }

  enum channel { LEFT, RIGHT };

  int operator()( int j, channel ch = LEFT ) const;

  void seek( int pos );
  int tell() const { return _bufpos; }
  int buf_size() const { return _bufsize; }

private:
  referer<stream> _file;

  int _channels;
  int _samplespersec; // в одном канале
  int _bitspersample; // в одном канале
  int _alignment;     // байтов на один отсчет обоих каналов
  int _datasize;      // всего данных в байтах
  int _datapos;       // позиция начала данных в файле в байтах

  int _bufpos;           // текущая позиция буфера в отсчетах
  int _bufsize;          // размер заполненной части буфера
  t_membuf<uchar> _data;

  int wait4data();
  int wait4fmt();
};

class riffwave_saver : public scope {
public:
  static void put( const char*, const vector&, int sps );

private:
  static void write_chunkhdr( referer<stream> file, const char *id, uint32 sz );
  static void write_fmt( referer<stream> file, int sps );
  static void write_data( referer<stream> file, const vector& );
  static void write_align( referer<stream> file );
};

}; // namespace lwml

#endif // _RIFF_
