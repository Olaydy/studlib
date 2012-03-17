#include <stdio.h>
#include <stdlib.h>

#include "riff.h"

int main( int argc, char *argv[] ){
  if( argc != 2 ){
    printf("usage: test_riff wav-file-name");
    exit(1);
  }

  // ������� ������
  riffwave_reader in(argv[1]);

  // �������� ����� ���������� � �����
  printf("channels=%d\n", in.channels());
  printf("bitspersample=%d\n", in.bitspersample());

  int len = in.data_size();
  int sps = in.samplespersec();
  printf("len=%d\n", len);
  printf("sps=%d\n", sps);

  // ������ ������ � ������
  int* data = static_cast<int*>(malloc(len * sizeof(int)));
  if( data == 0 ){
    printf("can't allocate buffer\n");
    exit(1);
  }    
  for( int j = 0; j < len; j++ )
    data[j] = in(j, riffwave_reader::LEFT);

  // ������� ������ � ��������� ����
  FILE* file = fopen("wave.dat", "wt");
  for( int j = 0; j < len; j++ )
    fprintf(file, "%d\n", data[j]);
  fclose(file);

  // ������� �������� ��������� ������
  int min = data[0];
  int max = data[0];
  for( int j = 0; j < len; j++ ){
    if( data[j] < min )
      min = data[j];
    if( data[j] > max )
      max = data[j];
  }
  printf("min=%d max=%d\n", min, max);

  // ���������������� ������ �� �������� [-1, 1]
  double* rdata = static_cast<double*>(malloc(len * sizeof(double)));
  if( rdata == 0 ){
    printf("can't allocate buffer\n");
    exit(1);
  }    
  for( int j = 0; j < len; j++ )
    rdata[j] = 2.0 * static_cast<double>(data[j] - min) / static_cast<double>(max - min) - 1.0;

  // ��������� ��������� ������
  const double VOL_MULT = 0.05;
  for( int j = 0; j < len; j++ )
    rdata[j] *= VOL_MULT;

  // ������� ������ � ����� wav-����
  save_as_riff("out.wav", rdata, len, sps);
}
