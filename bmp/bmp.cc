#include "bmp.h"

#include <stdlib.h>

// error handler

namespace {
  void std_bmp_error( const char* msg )
  {
    fprintf(stderr, "bmp: error: %s\n", msg);
    exit(1);
  }

  bmp_error_t bmp_error = std_bmp_error;
}

void setup_bmp_error_handler( bmp_error_t handler )
{
  bmp_error = handler;
}

// bmp_rgb

const double RED_GRAY_WEIGHT   = 0.299;
const double GREEN_GRAY_WEIGHT = 0.587;
const double BLUE_GRAY_WEIGHT  = 0.114;

uchar bmp_rgb::gray() const
{
  return static_cast<uchar>(RED_GRAY_WEIGHT * _red + GREEN_GRAY_WEIGHT * _green + BLUE_GRAY_WEIGHT * _blue);
}

// constants

const int FILEHEADERSIZE         =  14;
const int IMAGEHEADERSIZE        =  40;
const int HEADERSIZE             =  FILEHEADERSIZE + IMAGEHEADERSIZE;
const int MAGIC                  =  0x4D42; // magic word = "BM"
const int PLANES                 =  1;
const int COMPRESSION            =  0;

const int MAGIC_OFFSET           =  0;
const int IMAGEHEADERSIZE_OFFSET =  14;
const int PLANES_OFFSET          =  26;
const int COMPRESSION_OFFSET     =  30;
const int IMAGEOFFSET_OFFSET     =  10;
const int WIDTH_OFFSET           =  18;
const int HEIGHT_OFFSET          =  22;
const int BITSPERPIXEL_OFFSET    =  28;
const int IMAGESIZE_OFFSET       =  34;
const int PALETTE_OFFSET         =  54;

const int TRUECOLOROFFSET        =  HEADERSIZE;
const int TRUECOLORBITSPERPIXEL  =  24;

const int PALELEMSIZE            =  4;

// bmp_pal

void bmp_pal::read( FILE* file, int size )
{
  if( size == 0 )
    _len = 0;
  else{
    _len = size;
    if( fseek(file, PALETTE_OFFSET, SEEK_SET) != 0 )
      bmp_error("can't seek to palette");
    if( fread(_pal, PALELEMSIZE, _len, file) != static_cast<uint>(_len) )
      bmp_error("can't read palette");
  }
}

void bmp_pal::write( FILE* file )
{
  if( _len != 0 ){
    if( fseek(file, PALETTE_OFFSET, SEEK_SET) != 0 )
      bmp_error("can't seek to palette");
    if( fwrite(_pal, PALELEMSIZE, _len, file) != static_cast<uint>(_len) )
      bmp_error("can't write palette");
  }
}

bmp_pal::bmp_pal( int size )
{
  if( size < 0 )
    bmp_error("incorrect palette size");
  if( size > MAXPALSIZE )
    bmp_error("too long bitmap palette");
  _len = size;
  if( _len != 0 ){
    for( int j = 0; j < _len; j++ ){
      uint c = (j * (MAXPALSIZE-1)) / (_len-1);
      _pal[j] = (c << 16) | (c << 8) | c;
    }
  }
}

bmp_pal::bmp_pal( const char* fname, int sz )
{
  if( sz < 0 )
    bmp_error("negative palette size");
  if( sz > MAXPALSIZE )
    bmp_error("too long bitmap palette");
  FILE* file = fopen(fname, "rb");
  if( file == 0 )
    bmp_error("can't open file");
  read(file, sz);
  fclose(file);
}

bmp_rgb bmp_pal::get( int idx ) const
{
  if( idx < 0 || idx > _len-1 )
    bmp_error("index out of palette");
  uint col = _pal[idx];
  return bmp_rgb((col & 0xff0000) >> 16, (col & 0x00ff00) >> 8, col & 0x0000ff);
}

void bmp_pal::put( int idx, bmp_rgb rgb )
{
  if( idx < 0 || idx > _len-1 )
    bmp_error("index out of palette");
  _pal[idx] = (rgb.red() << 16) | (rgb.green() << 8) | rgb.blue();
}

// bmp_header

void bmp_header::write16( FILE* file, int x )
{
  uint16 buf = x;
  if( fwrite(&buf, sizeof(buf), 1, file) != 1 )
    bmp_error("can't write header");
}

void bmp_header::write32( FILE* file, int x )
{
  uint32 buf = x;
  if( fwrite(&buf, sizeof(buf), 1, file) != 1 )
    bmp_error("can't write header");
}

int bmp_header::read16( FILE* file, int ofs )
{
  uint16 x;
  if( fseek(file, ofs, SEEK_SET) != 0 )
    bmp_error("can't seek to header");
  if( fread(&x, sizeof(x), 1, file) != 1 )
    bmp_error("can't read header");
  return x;
}

int bmp_header::read32( FILE* file, int ofs )
{
  uint32 x;
  if( fseek(file, ofs, SEEK_SET) != 0 )
    bmp_error("can't seek to header");
  if( fread(&x, sizeof(x), 1, file) != 1 )
    bmp_error("can't read header");
  return x;
}

// Константные поля:
//   magic, image header size, planes, compression
// Безразличные поля:
//   file size, reserved, image size,
//   xbitspermeter, xbitspermeter,
//   colors, importantcolors,
// Важные поля:
//   image offset, width, height, bits per pixel

void bmp_header::write( FILE* file )
{
  int imageoffset = _palsize * PALELEMSIZE + HEADERSIZE;
  write16(file, MAGIC);                          // magic
  write32(file, 0);                              // file size
  write32(file, 0);                              // reserved
  write32(file, imageoffset);                    // image offset
  write32(file, IMAGEHEADERSIZE);                // image header size
  write32(file, _width);                         // width
  write32(file, _height);                        // height
  write16(file, PLANES);                         // planes
  write16(file, _bitsperpixel);                  // bits per pixel
  write32(file, COMPRESSION);                    // compression
  write32(file, 0);                              // image size
  write32(file, 0);                              // xbitspermeter
  write32(file, 0);                              // ybitspermeter
  write32(file, 0);                              // colors
  write32(file, 0);                              // importantcolors
}

void bmp_header::read( FILE* file )
{
  if( read16(file, MAGIC_OFFSET) != MAGIC )
    bmp_error("wrong magic word");

  if( read32(file, IMAGEHEADERSIZE_OFFSET) != IMAGEHEADERSIZE )
    bmp_error("unknown bitmap type");

  if( read16(file, PLANES_OFFSET) != PLANES )
    bmp_error("wrong planes number");

  if( read32(file, COMPRESSION_OFFSET) != COMPRESSION )
    bmp_error("can't read compressed bitmap");

  int imageoffset = read32(file, IMAGEOFFSET_OFFSET);
  _palsize = (imageoffset - HEADERSIZE) / PALELEMSIZE;
  _width = read32(file, WIDTH_OFFSET);
  _height = read32(file, HEIGHT_OFFSET);
  _bitsperpixel = read16(file, BITSPERPIXEL_OFFSET);
  _bpl = width2bpl(_width);
}

int bmp_header::width2bpl( int width )
{
  int bpl = 0; // has no mean!
  switch( _bitsperpixel ){
    case 1:
      bpl = (width + 7) / 8;
      break;
    case 4:
      bpl = (width + 1) / 2;
      break;
    case 8:
      bpl = width;
      break;
    case 24:
      bpl = width * 3;
      break;
    case 32:
      bpl = width * 4;
      break;
    default:
      bmp_error("unexpected depth of color");
  }
  return (bpl + 3) & ~0x03;
}

int bmp_header::palsize2bpp( int ps )
{
  switch( ps ){
    case 0:   return 24;
    case 2:   return 1;
    case 16:  return 4;
    case 256: return 8;
    default:
      bmp_error("incorrect palette size (may be 0, 2, 16, 256 only)");
      return 0; // has no mean!
  }
}

bmp_header::bmp_header( const char* fname )
{
  FILE* file = fopen(fname, "rb");
  if( file == 0 )
    bmp_error("can't open file");
  read(file);
  fclose(file);
}

bmp_header::bmp_header( int height, int width, int palsize )
{
  _palsize = palsize;
  _width = width;
  _height = height;
  _bitsperpixel = palsize2bpp(_palsize);
  _bpl = width2bpl(_width);
}

// bitmap

uchar bitmap::read_byte( FILE* file )
{
  uchar x;
  if( fread(&x, sizeof(x), 1, file) != 1 )
    bmp_error("can't read header");
  return x;
}

void bitmap::write_byte( FILE* file, uchar x )
{
  if( fwrite(&x, sizeof(x), 1, file) != 1 )
    bmp_error("can't write header");
}

void bitmap::read( FILE* file )
{
  if( fseek(file, HEADERSIZE + _hdr.palsize() * PALELEMSIZE, SEEK_SET) != 0 )
    bmp_error("can't seek to data");
  int byte_num = _hdr.bytesperline() * _hdr.height();
  for( int j = 0; j < byte_num; j++ )
    _image[j] = read_byte(file);
}

void bitmap::write( FILE* file )
{
  if( fseek(file, HEADERSIZE + _hdr.palsize() * PALELEMSIZE, SEEK_SET) != 0 )
    bmp_error("can't seek to data");
  int byte_num = _hdr.bytesperline() * _hdr.height();
  for( int j = 0; j < byte_num; j++ )
    write_byte(file, _image[j]);
}

bitmap::bitmap( const char* fname )
  : _hdr(fname),
    _pal(fname, _hdr.palsize())
{
  _image = static_cast<uchar*>(malloc(_hdr.bytesperline() * _hdr.height()));
  if( _image == 0 )
    bmp_error("can't allocate memory buffer");
  FILE* file = fopen(fname, "rb");
  if( file == 0 )
    bmp_error("can't open file");
  read(file);
}

bitmap::bitmap( int height, int width, const bmp_pal& pal )
  : _hdr(height, width, pal.len()),
    _pal(pal)
{
  _image = static_cast<uchar*>(malloc(_hdr.bytesperline() * _hdr.height()));
  if( _image == 0 )
    bmp_error("can't allocate memory buffer");
}

bitmap::~bitmap()
{
  free(_image);
}

void bitmap::save( const char* name )
{
  FILE* file = fopen(name, "wb");
  if( file == 0 )
    bmp_error("can't open file");
  _hdr.save(file);
  _pal.save(file);
  write(file);
}

uchar bitmap::getcolidx( int lidx, int x ) const
{
  uchar sh;
  int bpp = _hdr.bitsperpixel();
  switch( bpp ){
    case 1:
      lidx += x / 8;
      sh = 7 - x % 8;
      return (_image[lidx] & (1 << sh)) >> sh;
    case 4:
      lidx += x / 2;
      sh = (x & 1) ? 0 : 4;
      return (_image[lidx] & (0x0f << sh)) >> sh;
    case 8:
      lidx += x;
      return _image[lidx];
    default:
      bmp_error("unexpected depth of color");
      return 0; // has no mean!
  }
}

void bitmap::setcolidx( int lidx, int x, int cidx )
{
  uchar sh;
  int bpp = _hdr.bitsperpixel();
  switch( bpp ){
    case 1:
      lidx += x / 8;
      sh = 7 - x % 8;
      _image[lidx] &= ~(1 << sh);
      _image[lidx] |= (cidx & 1) << sh;
      break;
    case 4:
      lidx += x / 2;
      sh = (x&1) ? 0 : 4;
      _image[lidx] &= ~(0x0f << sh);
      _image[lidx] |= (cidx & 0x0f) << sh;
      break;
    case 8:
      lidx += x;
      _image[lidx] = (cidx & 0x00FF);
      break;
    default:
      bmp_error("unexpected depth of color");
  }
}

bmp_rgb bitmap::get( int y, int x ) const
{
  int lx = _hdr.width();
  int ly = _hdr.height();
  if( x < 0 || x > lx-1 )
    bmp_error("x-coord out of image size");
  if( y < 0 || y > ly-1 )
    bmp_error("y-coord out of image size");

  // переходим к естественной для bmp системе координат
  y = _hdr.height() - y - 1;

  // вычисляем смещение строки
  int lidx = y * _hdr.bytesperline();

  const int bpp = _hdr.bitsperpixel();

  if( bpp == 24 || bpp == 32 ){
    lidx += x * (bpp / 8); // добавляем смещение RGB
    return bmp_rgb(_image[lidx+2], _image[lidx+1], _image[lidx]);
  }else{
    uchar cidx = getcolidx(lidx, x); // получаем индекс цвета точки
    return _pal.get(cidx);
  }
}

void bitmap::put( int y, int x, bmp_rgb rgb )
{
  int lx = _hdr.width();
  int ly = _hdr.height();
  if( x < 0 || x > lx-1 )
    bmp_error("x-coord out of image size");
  if( y < 0 || y > ly-1 )
    bmp_error("y-coord out of image size");
  if( _hdr.bitsperpixel() != 24 && _hdr.bitsperpixel() != 32 )
    bmp_error("can't set RGB for image with palette");

  // переходим к естественной для bmp системе координат
  y = _hdr.height() - y - 1;

  // вычисляем смещение строки
  int lidx = y * _hdr.bytesperline();

  lidx += x * (_hdr.bitsperpixel() / 8); // добавляем смещение RGB
  _image[lidx+2] = rgb.red();
  _image[lidx+1] = rgb.green();
  _image[lidx] = rgb.blue();
}

void bitmap::fill( bmp_rgb rgb )
{
  if( _hdr.bitsperpixel() != 24 && _hdr.bitsperpixel() != 32 )
    bmp_error("can't set RGB for image with palette");

  int lx = _hdr.width();
  int ly = _hdr.height();
  for( int y = 0; y < ly; y++ ){
    int yy = _hdr.height() - y - 1;      // переходим к естественной для bmp системе координат
    int lidx = yy * _hdr.bytesperline(); // вычисляем смещение строки

    for( int x = 0; x < lx; x++ ){
      int lidx_x = lidx + x * (_hdr.bitsperpixel() / 8); // добавляем смещение RGB
      _image[lidx_x+2] = rgb.red();
      _image[lidx_x+1] = rgb.green();
      _image[lidx_x] = rgb.blue();
    }
  }
}

void bitmap::put( int y, int x, int cidx )
{
  int lx = _hdr.width();
  int ly = _hdr.height();
  if( x < 0 || x > lx-1 )
    bmp_error("x-coord out of image size");
  if( y < 0 || y > ly-1 )
    bmp_error("y-coord out of image size");
  if( _hdr.bitsperpixel() == 24 || _hdr.bitsperpixel() == 32 )
    bmp_error("can't use palette with truecolor bitmap");
  if( cidx < 0 || cidx >= _hdr.palsize() )
    bmp_error("color out of palette");

  // переходим к естественной для bmp системе координат
  y = _hdr.height() - y - 1;

  // вычисляем смещение строки
  int lidx = y * _hdr.bytesperline();

  // установить индекс цвета для точки
  setcolidx(lidx, x, cidx);
}

void bitmap::fill( int cidx )
{
  if( _hdr.bitsperpixel() == 24 || _hdr.bitsperpixel() == 32 )
    bmp_error("can't use palette with truecolor bitmap");
  if( cidx < 0 || cidx >= _hdr.palsize() )
    bmp_error("color out of palette");

  int lx = _hdr.width();
  int ly = _hdr.height();
  for( int y = 0; y < ly; y++ ){
    int yy = _hdr.height() - y - 1;      // переходим к естественной для bmp системе координат
    int lidx = yy * _hdr.bytesperline(); // вычисляем смещение строки

    for( int x = 0; x < lx; x++ )
      setcolidx(lidx, x, cidx);
  }
}
