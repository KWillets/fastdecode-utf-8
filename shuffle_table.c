#include <stdio.h>
#include <x86intrin.h>
#include <inttypes.h>

//#include "streamvbyte_shuffle_tables_decode.h"

static uint8_t shuffleTable[256][16];

#define extract(c,i) (3 & (c >> 2*i))

static void decoder_permutation(uint8_t *table) {
  uint8_t *p = table;
  for(int code = 0; code < 256; code++) {
    int byte = 0;
    for(int i = 0; i < 4; i++ ) {
      int c = extract(code, i);
      int j;
      for( j = c; j >= 0; j-- )
        p[j] = byte++;
      for( j=c+1; j < 4; j++ )
        p[j] = -1;
      p+=4;
    }
  }
}

void dump(char * xc) {
  
  for( int i=0; i<15; i++)
    printf("%2i,", xc[i]);
  printf("%2i", xc[15]);

}

int main() {
  decoder_permutation(&shuffleTable[0][0]);

  printf( "static uint8_t shuffleTable[256][16] = {\n");
  
  for(int i=0; i<256;i++) {
    dump((char *) &shuffleTable[i][0]);
    printf(i < 255 ? ",\n" :"\n};");
      }
  
}


