#include "riff.h"
#include "stdmem.h"
#include "mdefs.h"
#include "basex.h"
#include "filename.h"

/*#lake:stop*/

namespace lwml {

// definitions

const char RIFFCHUNK_ID[] = "RIFF";  // идентификатор чанка RIFF
const char FMTCHUNK_ID[]  = "fmt ";  // идентификатор чанка fmt
const char DATACHUNK_ID[] = "data";  // идентификатор чанка data

const int CHUNKID_SIZE  = 4;  // размер идентификатора чанка
const int CHUNKHDR_SIZE = 8;  // размер заголовка чанка

const char WAVEFORM_ID[]  = "WAVE"; // инентификатор WAVE-формы
const int WAVEFORMID_SIZE = 4;      // размер инентификатора WAVE-формы

const int PCMFORMAT_ID      = 1;   // идентификатор PCM-формата
const int FMTCHUNK_DATASIZE = 16;  // размер данных чанка fmt
const int FMTCHUNK_SIZE = CHUNKHDR_SIZE + FMTCHUNK_DATASIZE; // (заголовок + данные)

// out parameters

const int OUT_CHANNELS       = 1;  // число каналов при выводе
const int OUT_BYTESPERSAMPLE = 2;  // байт на отсчет при выводе
const int OUT_BITSPERSAMPLE  =  OUT_BYTESPERSAMPLE * 8;
const int OUT_ALIGNMENT = OUT_CHANNELS * OUT_BYTESPERSAMPLE; // число байтов на отсчет (во всех каналах)

inline int OUT_BYTESPERSEC( int sps )
{
  return OUT_CHANNELS * sps * OUT_BYTESPERSAMPLE;
}

inline int DATA_SIZE( int size )
{
  return OUT_CHANNELS * size * OUT_BYTESPERSAMPLE;
}

inline int DATACHUNK_SIZE( int size )
{
  return CHUNKHDR_SIZE + DATA_SIZE(size);
}

// common

inline int ALIGN2EVEN( int size )
{
  return (size & 0x01) ? size + 1 : size;
}

inline bool ISODD( int size )
{
  return size & 0x01;
}

// параметры сигнала

const int PCM8_MIN = 0;
const int PCM8_MAX = 255;
const int PCM8_MID = 128;

const int PCM16_MIN = -32768;
const int PCM16_MAX = 32767;
const int PCM16_MID = 0;

// chunk header

class chunkhdr : public value {
public:
  chunkhdr() {}
  chunkhdr( referer<stream> file ) { read(file); }

  void read( referer<stream> );

  void skipdata( referer<stream> );

  bool is_fmt() const  { return isid(FMTCHUNK_ID); }
  bool is_data() const { return isid(DATACHUNK_ID); }
  bool is_riff() const { return isid(RIFFCHUNK_ID); }
  int  size() const { return _sz; }

private:
  char   _id[CHUNKID_SIZE];  // идентификатор чанка
  uint32 _sz;                // размер данных в байтах

  bool isid( const char* id ) const;
};

void chunkhdr::read( referer<stream> file )
{
  file->read(_id, sizeof(_id));
  file->read(&_sz, sizeof(_sz));
}

void chunkhdr::skipdata( referer<stream> file )
{
  file->skip(_sz);
}

bool chunkhdr::isid( const char *id ) const
{
  return strncmp(_id, id, CHUNKID_SIZE) == 0;
}

// WAVE-form hdr

class waveformhdr : public value {
public:
  waveformhdr( referer<stream> );

  bool is_wave() const;

private:
  char _id[WAVEFORMID_SIZE];
};

waveformhdr::waveformhdr( referer<stream> file )
{
  file->read(_id, WAVEFORMID_SIZE);
}

bool waveformhdr::is_wave() const
{
  return strncmp(_id, WAVEFORM_ID, WAVEFORMID_SIZE) == 0;
}

// fmt-chunk

class fmtchunk : public value {
public:
  fmtchunk( referer<stream> file, int size );

  int channels() const { return _channels; }
  int samplespersec() const { return _samplespersec; }
  int bitspersample() const { return _bitspersample; }
  int alignment() const { return _alignment; }

private:
  uint16 _formattag;         // категория формата
  uint16 _channels;          // число каналов
  uint16  _bitspersample;    // разрядность сампла (в одном канале)
  uint32 _samplespersec;     // частота дискретизации (самплов в секунду в одном канале)
  uint32 _bytespersec;       // число байт в секунду для всех каналов
  uint16 _alignment;         // выравнивание данных
  // _bytespersec = (_channels * _samplespersec * _bitspersample)/8
  // _alignment = (nChannels * nBitsPerSample)/8
};

fmtchunk::fmtchunk( referer<stream> file, int size )
{
  if( size < FMTCHUNK_DATASIZE )
    throw ex_riff("incorrect data size in fmt-chunk");

  file->read(&_formattag, sizeof(_formattag));
  file->read(&_channels, sizeof(_channels));
  file->read(&_samplespersec, sizeof(_samplespersec));
  file->read(&_bytespersec, sizeof(_bytespersec));
  file->read(&_alignment, sizeof(_alignment));
  file->read(&_bitspersample, sizeof(_bitspersample));

  if( _formattag != PCMFORMAT_ID )
    throw ex_riff("file format seems not to be a PCM");

  if( size > FMTCHUNK_DATASIZE )
    file->skip(size-FMTCHUNK_DATASIZE);
}

// riff wave reader

int riffwave_reader::wait4data()
{
  chunkhdr chh;
  while( 1 ){
    chh.read(_file);
    if( chh.is_data() )
      break;
    else
      chh.skipdata(_file);
  }
  return chh.size();
}

int riffwave_reader::wait4fmt()
{
  chunkhdr chh;
  while( 1 ){
    chh.read(_file);
    if( chh.is_fmt() )
      break;
    else
      chh.skipdata(_file);
  }
  return chh.size();
}

void riffwave_reader::seek( int pos )
{
  test_index(pos, data_size());
  _bufpos = pos;
  _bufsize = t_min(data_size() - pos, _data.len() / _alignment);
  _file->seek(_datapos + pos * _alignment);
  _file->read(_data.access_raw_data(), _bufsize * _alignment);
}

riffwave_reader::riffwave_reader( const char *nm, int bufsize )
{
  // открыть файл
  _file = stream::create(nm, fmREAD, fmBINARY);

  // проверить чанк RIFF
  chunkhdr chh(_file);
  if( !chh.is_riff() )
    throw ex_riff("bad riff-chunk, <%s>", nm);

  // проверить чанк WAVE
  waveformhdr wfh(_file);
  if( !wfh.is_wave() )
    throw ex_riff("bad wave-chunk, <%s>", nm);

  // дождаться чанка fmt и прочитать
  int fmtsize = wait4fmt();
  fmtchunk fmt(_file, fmtsize);
  _channels = fmt.channels();
  _samplespersec = fmt.samplespersec();
  _bitspersample = fmt.bitspersample();
  if( _bitspersample != 8 && _bitspersample != 16 )
    throw ex_riff("can't read RIFF WAVE file with given bitspersample (%d)", _bitspersample);
  _alignment = fmt.alignment();
  if( _alignment * 8 != _channels * _bitspersample )
    throw ex_riff("incorrect value of alignment field in RIFF fmt chunk");

  // дождаться данных и прочитать
  _datasize = wait4data();
  _datapos = _file->tell();
  if( _datasize % _alignment != 0 )
    throw ex_riff("incorrect data size in RIFF data chunk");

  if( bufsize > 0 ) // задан размер буфера
    _data.resize(bufsize * _alignment);
  else // буфер вмещает все данные из файла
    _data.resize(_datasize);

  seek(0);
}

void riffwave_reader::printinfo( referer<stream> file ) const
{
  file->printf("channels=%d\n", _channels);
  file->printf("samplespersec=%d\n", _samplespersec);
  file->printf("bitspersample=%d\n", _bitspersample);
  file->printf("datasize=%d\n", _datasize);
}

int riffwave_reader::operator()( int j, channel ch ) const
{
  test_index(j, buf_size());
  if( ch == RIGHT && _channels != 2 )
    fail_assert("can't read right channel for mono RIFF WAVE");

  int sample = j * _alignment;
  if( ch == RIGHT ) sample += (_bitspersample == 8) ? 1 : 2;
  uint16 buf = (_bitspersample == 8) ? _data[sample] : (_data[sample+1] << 8) | _data[sample];
  return static_cast<int16>(buf); //!! TODO: исправить
}

// riff wave saver

void riffwave_saver::write_chunkhdr( referer<stream> file, const char *id, uint32 sz )
{
  file->write(id, CHUNKID_SIZE);
  file->write(&sz, sizeof(uint32));
}

void riffwave_saver::write_fmt( referer<stream> file, int sps )
{
  uint16 formattag = PCMFORMAT_ID;           // категори формата
  uint16 channels = OUT_CHANNELS;            // число каналов
  uint32 samplespersec = sps;                // частота дискретизации
  uint16 bitspersample = OUT_BITSPERSAMPLE;  // разрдность дискретизации
  uint32 bytespersec = OUT_BYTESPERSEC(sps); // число байт в секунду
  uint16 alignment = OUT_ALIGNMENT;          // выравнивание данных

  file->write(&formattag, sizeof(formattag));
  file->write(&channels, sizeof(channels));
  file->write(&samplespersec, sizeof(samplespersec));
  file->write(&bytespersec, sizeof(bytespersec));
  file->write(&alignment, sizeof(alignment));
  file->write(&bitspersample, sizeof(bitspersample));
}

void riffwave_saver::write_data( referer<stream> file, const vector& v )
{
  real min = v.min();
  real max = v.max();
  if( max == min )
    max = min + 1.0;
  if( min == min + 1.0 || max == max + 1.0 )
    throw ex_sing();

  for( int j = 0; j < v.len(); j++ ){
    uint16 buf = scale::interval(v[j], min, max, PCM16_MIN, PCM16_MAX);
    uchar lo = buf & 0x00FF;
    uchar hi = (buf >> 8) & 0x00FF;
    file->write(&lo, sizeof(lo));
    file->write(&hi, sizeof(hi));
  }
}

void riffwave_saver::write_align( referer<stream> file )
{
  uchar buf = 0;
  file->write(&buf, sizeof(uchar));
}

void riffwave_saver::put( const char* name, const vector& x, int sps )
{
  referer<stream> file = stream::create(name, fmWRITE, fmBINARY);

  int riff_size = WAVEFORMID_SIZE + FMTCHUNK_SIZE + DATACHUNK_SIZE(x.len());
  // RIFF-chunk
  write_chunkhdr(file, RIFFCHUNK_ID, ALIGN2EVEN(riff_size));
  // WAVE-form
  file->write(WAVEFORM_ID, WAVEFORMID_SIZE);
  // fmt_chunk
  write_chunkhdr(file, FMTCHUNK_ID, FMTCHUNK_DATASIZE);
  write_fmt(file, sps);
  // data_chunk
  write_chunkhdr(file, DATACHUNK_ID, DATA_SIZE(x.len()));
  write_data(file, x);
  // alignment
  if( ISODD(riff_size) ) write_align(file);
}

// данные хрантся в старших bitspersample битах

}; // namespace lwml
