#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benchmark.h"
#include "utf8_code.h"

#include <x86intrin.h>
/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

void populate(char *data, size_t N) {
  for (size_t i = 0; i < N; i++)
    data[i] = rand() & 0x7f;
}

void populate3(char *data, size_t N) {
  int i;
  for (i = 0; i+3 < N; i+=3)
    memcpy(data+i, "ㄱ", 3);

  for(; i < N; i++)
    data[i]='a';
}


size_t decode(char *txt, int length, uint32_t *out)
{
  uint32_t *ptmp = out;
  printf("N %d txt %4s\n", length, txt + length -4);
  int l = utf32_decode((uint8_t *) txt, length, &ptmp);
  printf("done\n%8x\n%8x\n%8x\n%8x\n", *(ptmp-4),*(ptmp-3),*(ptmp-2),*(ptmp-1));
  printf("nout %zu\n", ptmp-out);
  return l;
}

void demo(size_t N) {
  printf("string size = %zu \n", N);
  char *data = (char *)malloc(N);
  uint32_t *out = (uint32_t *)malloc(N*sizeof(uint32_t));
  
  int expected = 0; // it is all ascii?
  int repeat = 5;
  printf("ascii\n");

  BEST_TIME(decode(data, N, out), expected, populate(data, N), repeat, N,
            true);

  printf("length 3 utf-8, with filler ascii at end\n");
  BEST_TIME(decode(data, N, out), expected, populate3(data, N), repeat, N,
            true);
  /*
#ifdef __linux__
  BEST_TIME_LINUX(validate_utf8_fast(data, N), expected, populate(data, N),
                  repeat, N, true);
#endif
  */
  printf("\n\n");

  free(data);
}

int main() {
  demo(65536);
  printf("We are feeding ascii so it is always going to be ok.\n");
}
