#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

int main( int argc, char *argv[] ){
  if( argc != 2 ){
    printf("usage: test_riff bmp-file-name");
    exit(1);
  }

  // создаем объект
  bitmap in(argv[1]);

  // печатаем общую информацию о файле
  printf("width=%d\n", in.width());
  printf("height=%d\n", in.height());
  printf("bpp=%d\n", in.bpp());

  // обрабатываем изображение
  for( int y = 0; y < in.height(); y++ ){
    for( int x = 0; x < in.width(); x++ ){
      bmp_rgb rgb = in.get(y, x);
      if( rgb.gray() > 128 )
        rgb.set_blue(0);
      in.put(y, x, rgb);
    }
  }

  // сохраняем изображение
  in.save("out.bmp");
}
