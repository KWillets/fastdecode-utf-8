#include <stdio.h>
#include <string.h>
#include <x86intrin.h>
#include <inttypes.h>

#include "shuffle_table.h"
#include "utils.h"

#define mask _mm_setr_epi8(0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
		      0x3F,0x3F,0x3F,0x3F,\
		      0x1F,0x1F,\
		      0x0F, 0x07)



#define len   _mm_setr_epi8(0,0,0,0,0,0,0,0,-1,-1,-1,-1,1,1,2,3)
#define iota1 _mm_setr_epi8(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)

// shared routines
__m128i compress( __m128i bitfields ) {
  const __m128i pack16  = _mm_maddubs_epi16(bitfields, _mm_set1_epi32(0x40014001));
  return _mm_madd_epi16(pack16, _mm_set1_epi32(0x10000001));
}

__m128i make_upper4(__m128i  utf8) {return _mm_and_si128(_mm_srli_epi64(utf8, 4), _mm_set1_epi8(0x0F));}

__m128i shift( __m128i x, int i ) {
  char ix[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  return _mm_shuffle_epi8(x, _mm_loadu_si128((__m128i *)(ix + i)));
}

int utf32_code_ptr(void *p, uint32_t **pout) {
  __m128i utf8 = _mm_loadu_si128(p); 
  __m128i upper4 = make_upper4(utf8);
  __m128i lengths = _mm_shuffle_epi8(len, upper4);

  __m128i P = _mm_adds_epu8(lengths, iota1);
  __m128i P2 = _mm_shuffle_epi8(P,P);
  __m128i P4 = _mm_shuffle_epi8(P2,P2);

  char jmp[16];
  _mm_storeu_si128((__m128i *) jmp, P4);

  // iterate through jmp
  int i, multibyte = 0;
  __m128i masked;
  uint8_t bcode[16];

  for(i = 0; i < 16 && jmp[i] > i; i = jmp[i])
    {
      int bytes = jmp[i] - i;
      __m128i mout;
      if( bytes == 4 ) {
	mout = _mm_cvtepu8_epi32(shift(utf8, i));
      } else {
	if( !multibyte ) {
	  __m128i code12 = _mm_or_si128(lengths,_mm_slli_epi64(_mm_shuffle_epi8(lengths, P), 2));  
	  __m128i code =   _mm_or_si128(code12,_mm_slli_epi64(_mm_shuffle_epi8(code12, P2), 4));
	  _mm_storeu_si128( (__m128i *)bcode, code );
	  masked = _mm_and_si128(_mm_shuffle_epi8(mask,upper4), utf8);
	  multibyte = 1;
	}
	
	__m128i Shuf = *(__m128i *) &shuffleTable[bcode[i]][0];
	__m128i bitfields = _mm_shuffle_epi8(shift(masked, i), Shuf);
	mout = compress(bitfields);
      }
      _mm_storeu_si128((__m128i *)*pout, mout);
      *pout += 4;
    }
  return i;
}

size_t utf32_decode(uint8_t *txt, int length, uint32_t **pout)
{
  int n = 0;
  uint8_t *p = txt;
  int l = length;

  while((l > 16) && ((n = utf32_code_ptr(p, pout))))
    {
      p += n;
      l -= n;
    }

  while(l > 0) // 1-2 iterations to consume end
    {
      uint8_t inbuf[16];
      uint32_t outbuf[16];
      uint32_t *pbuf = outbuf;
      memset(inbuf, 0, 16);
      memcpy(inbuf, p, l);
      int nused = utf32_code_ptr(inbuf, &pbuf);
      int nout = pbuf-outbuf;
      int nextra = 0;
      if( nused >= l ) {
	nextra = nused - l; // one byte per extra
	l = 0;
      } else {
	nextra = 0;
	p += nused;
	l -= nused;
      }
      
      for(int i = 0; i < nout - nextra; i++) {
	**pout = outbuf[i];
	(*pout)++;
      }
    }
  return l;
}




