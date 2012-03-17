// windows bitmap

#ifndef _BMP_
#define _BMP_

#include <stdio.h>

#include "cdefs.h"

// Тип функции-обработчика ошибок.
typedef void (*bmp_error_t)( const char* msg );

// Установка обработчика ошибок.
// По умолчанию обработчик ошибок выводит сообщение на stdout
// и завершает выполнение программы.
void setup_bmp_error( bmp_error_t );

// Представляет цвет точки на картинке формата BMP
// Цвет представлен в виде трех компонент в цветовой схеме RGB.
// Каждая цветовая компонента представлена однобайтовым значением.

class bmp_rgb {
public:
  // Иинициализация по умолчанию
  // При такой инициализации получается черная точка
  bmp_rgb()
    : _red(0), _green(0), _blue(0) {}

  // Инициализация явно заданными значениями
  bmp_rgb( uchar r, uchar g, uchar b )
    : _red(r), _green(g), _blue(b) {}

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

  // Получить целочисленное значение яркости
  uchar gray() const;

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

class bmp_pal {
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
  void save( FILE* file ) { write(file); }

private:
  int _len;
  uint32 _pal[MAXPALSIZE];

  // чтение палитры заданной длины из файла по заданному смещению
  // полностью заново конструирует палитру
  // используется только методами класса bitmap
  void read( FILE* file, int size );
  void write( FILE* file );
};

// Класс реализует чтение и запись заголовка bmp-файла

class bmp_header {
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
  void save( FILE* file ) { write(file); }

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

  void write16( FILE* file, int );
  void write32( FILE* file, int );
  int read16( FILE* file, int ofs );
  int read32( FILE* file, int ofs );

  void write( FILE* file );
  void read( FILE* file );

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

class bitmap {
public:
  // Чтение изображения из файла
  bitmap( const char* fname );

  // Создание изображение с заданными параметрами
  // height - высота создаваемого изображения
  // width - ширина создаваемого изображения
  // pal - палитра создаваемого изображения
  bitmap( int height, int width, const bmp_pal& pal );

  ~bitmap();

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
  uchar* _image;

  uchar read_byte( FILE* );
  void write_byte( FILE*, uchar );

  void read( FILE* );
  void write( FILE* );

  uchar getcolidx( int lidx, int x ) const;
  void  setcolidx( int lidx, int x, int cidx );
};

#endif // _BMP_
