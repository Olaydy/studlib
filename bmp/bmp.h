// Работа с файлами в формате windows bitmap
// lwml, (c) ltwood

#ifndef _BMP_
#define _BMP_

#include "defs.h"
#include "mdefs.h"
#include "stream.h"
#include "t_membuf.h"
#include "rgb.h"

/*#lake:stop*/

namespace lwml {

// Исключение при ошибке интерпретации содержимого битмапа

DEF_EX_TYPE_MSG(ex_integrity, ex_bmp, "incorrect bitmap");

// Представляет цвет точки на картинке формата BMP
// Цвет представлен в виде трех компонент в цветовой схеме RGB.
// Каждая цветовая компонента представлена однобайтовым значением.

class bmp_rgb : public value {
public:
  // Иинициализация по умолчанию
  // При такой инициализации получается черная точка
  bmp_rgb()
    : _red(0), _green(0), _blue(0) {}

  // Инициализация явно заданными значениями
  bmp_rgb( uchar r, uchar g, uchar b )
    : _red(r), _green(g), _blue(b) {}

  // Инициализация по обычному вещественному цвету
  bmp_rgb( const rgbcolor& rgb )
    : _red(fpr::lround(255.0 * rgb.red())),
      _green(fpr::lround(255.0 * rgb.green())),
      _blue(fpr::lround(255.0 * rgb.blue()))
  {}

  // Одновременная установка всех цветовых компонент
  void set( uchar r, uchar g, uchar b ){
    _red = r; _green = g; _blue = b;
  }

  // Получение значений цветовых компонент
  uchar red()   const { return _red; }
  uchar green() const { return _green; }
  uchar blue()  const { return _blue; }

  // Изменение значений цветовых компонент
  // Функции возвращают ссылку на переменную,
  // хранящую значение цветовой компоненты

  void set_red( uchar r )   { _red = r; }
  void set_green( uchar g ) { _green = g; }
  void set_blue( uchar b )  { _blue = b; }

  // Получить обычное вещественное представление цвета
  rgbcolor get_rgb() const { return rgbcolor(_red/255.0, _green/255.0, _blue/255.0); }

  // Получить целочисленное значение яркости
  uchar gray() const { return static_cast<uchar>(fpr::lround(255.0 * get_rgb().gray())); }

private:
  uchar _red, _green, _blue;
};

// Максимальная длина палитры в файле BMP

const int MAXPALSIZE = 256;

// Палитра, входящая в состав файла формата BMP
// Палитра отсутствует в случае полноцветной картинки
// (true color, 24 бита на пиксел).
// Физическое представление палитры совпадает с тем,
// как она хранится в файле формата BMP.
// Выделение цветовых компонентов осуществляется в момент обращения.

class bmp_pal : public value {
public:
  // Создается серая палитра с растущим значением яркости
  // size - размер палитры
  // При size == 0 создается пустая палитра,
  // соответствующая полноцветному изображению.
  bmp_pal( int size = 0 );

  // Прочитать палитру из файла формата BMP
  // fname - имя файла
  // sz - размер читаемой палитры
  bmp_pal( const char* fname, int sz );

  // Получение размера палитры
  // Возвращает размер палитры в записях
  int len() const { return _len; }

  // Получить элемент палитры с заданным индексом
  bmp_rgb get( int idx ) const;

  // Установка значения элемента палитры с заданным индексом
  void put( int idx, bmp_rgb rgb );

  // Запись палитры в файл BMP
  // file - двоичный файл, в который производится запись
  void save( referer<stream> file ) { write(file); }

private:
  int _len;
  uint32 _pal[MAXPALSIZE];

  // чтение палитры заданной длины из файла по заданному смещению
  // полностью заново конструирует палитру
  // используется только методами класса bitmap
  void read( referer<stream> file, int size );
  void write( referer<stream> file );
};

// Класс реализует чтение и запись заголовка bmp-файла

class bmp_header : public value {
public:
  // Прочитать заголовок из файла.
  // fname - имя файла, из которого производится чтение
  bmp_header( const char* fname );

  // Создание заголовка по параметрам
  // height - высота изображения
  // width - ширина изображения
  // palsize - размер палитры
  bmp_header( int height, int width, int palsize );

  // Сохранить заголовок в двоичный файл
  void save( referer<stream> file ) { write(file); }

  // Получить размер палитры в записях
  int palsize() const { return _palsize; }

  // Получить ширину изображения в пикселах
  int width()   const { return _width; }

  // Получить высоту изображения в пикселах
  int height()  const { return _height; }

  // Получить глубину цвета в битах на пиксел
  int bitsperpixel() const { return _bitsperpixel; }

  // Получить ширину изображения в байтах
  int bytesperline() const { return _bpl; }

private:
  int _palsize;
  int _width, _height;
  int _bitsperpixel;
  int _bpl;

  void write16( referer<stream>, int );
  void write32( referer<stream>, int );
  int read16( referer<stream>, int ofs );
  int read32( referer<stream>, int ofs );

  void write( referer<stream> file );
  void read( referer<stream> file );

  int width2bpl( int width );
  int palsize2bpp( int );
};

// Изображение в формате BMP
// Поддержаны варианты с 1,4,8,24,32 битами на точку.
// При чтении изображения из файла корректно поддержан случай неполной палитры.
// При создании изображения должна быть задана полная палитра
// и при этом по размеру палитры определяется глубина цвета (бит на точку)
// по следующему правилу: 0->24, 2->1, 16->4, 256->8.
// Y-координата увеличивается при движении вниз.

class bitmap : public value {
public:
  // Чтение изображения из файла
  bitmap( const char* fname );

  // Создание изображение с заданными параметрами
  // height - высота создаваемого изображения
  // width - ширина создаваемого изображения
  // pal - палитра создаваемого изображения
  bitmap( int height, int width, const bmp_pal& pal );

  // Сохранение изображения в файле
  void save( const char* name );

  // Получение ширины изображения в пикселах
  int width() const { return _hdr.width(); }

  // Получение высоты изображения в пикселах
  int height() const { return _hdr.height(); }

  // Получение глубины цвета в битах на точку (1,4,8,24)
  int bpp() const { return _hdr.bitsperpixel(); }

  // Получить ссылку на палитру изображения
  bmp_pal& get_pal() { return _pal; }

  // Получить цвет точки с заданными координатами
  bmp_rgb get( int y, int x ) const;

  // Установить цвет точки в значение, заданное как истинный цвет (RGB).
  // Допустимо только для изображения с глубиной цвета 24 бита на точку.
  void put( int y, int x, bmp_rgb rgb );

  // Установить цвет всех точек в значение, заданное как истинный цвет (RGB).
  // Допустимо только для изображения с глубиной цвета 24 бита на точку.
  void fill( bmp_rgb rgb );

  // Установить цвет точки в значение, заданное индексом цвета в текущей палитре.
  // Допустимо только для изображения с глубиной цвета 1,4,8 бит на точку.
  // cidx - индекс цвета в текущей палитре.
  void put( int y, int x, int cidx );

  // Установить цвет всех точек в значение, заданное индексом цвета в текущей палитре.
  // Допустимо только для изображения с глубиной цвета 1,4,8 бит на точку.
  // cidx - индекс цвета в текущей палитре.
  void fill( int cidx );

private:
  bmp_header _hdr;
  bmp_pal _pal;
  t_membuf<uchar> _image;

  uchar read_byte( referer<stream> );
  void write_byte( referer<stream>, uchar );

  void read( referer<stream> );
  void write( referer<stream> );

  uchar getcolidx( int lidx, int x ) const;
  void  setcolidx( int lidx, int x, int cidx );
};

}; // namespace lwml

#endif // _BMP_
