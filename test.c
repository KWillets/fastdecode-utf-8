#include <stdio.h>
#include <string.h>
#include <x86intrin.h>
#include <inttypes.h>


#include "utf8_code.h"

void dump32(uint32_t *c, size_t cnt) {
  for(int i = 0; i < cnt; i++ )
    printf("%8x,", c[i]);
  printf("\n");
}

int main() {

  // reference:  https://unicode-table.com/en/3139/
  // 0x61, 0x3131, 0x62, 0x3134, 0x3137, 0x3139, ...
  uint8_t utf8[] = "aㄱbㄴcdefㄷㄹㅁㅂㅅㅇㅈㅊㅋㅌㅍㅎ";

  uint32_t out[1024];

  int nbytes = 0;
  uint32_t *pout = out;
  nbytes = utf32_code_ptr(utf8, &pout);
  printf("nbytes %d\n", nbytes);
  dump32(out, 8);

  
  uint8_t ascii[] = "abcdefghijklmnop";
  pout = out;
  nbytes = utf32_code_ptr(ascii, &pout);
  printf("nbytes %d\n", nbytes);
  dump32(out, 5);

  pout = out;
  uint8_t longtxt[] = "abcdefghijklmnopjㄱㄴㄷㄹㅁㅂㅅㅇㅈㅊㅋㅌㅍㅎz";
  uint8_t *plong = longtxt;
  utf32_decode(plong, sizeof(longtxt), &pout);

  dump32(out, pout-out);
}
