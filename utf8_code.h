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
static inline __m128i compress( __m128i bitfields ) {
  const __m128i pack16  = _mm_maddubs_epi16(bitfields, _mm_set1_epi32(0x40014001));
  return _mm_madd_epi16(pack16, _mm_set1_epi32(0x10000001));
}

static inline __m128i make_upper4(__m128i  utf8) {return _mm_and_si128(_mm_srli_epi64(utf8, 4), _mm_set1_epi8(0x0F));}

static inline __m128i shift( __m128i x, int i ) {
  static char ix[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  return _mm_shuffle_epi8(x, _mm_loadu_si128((__m128i *)(ix + i)));
}

static inline void storeq( uint32_t *out, __m128i utf8, int i, uint8_t *bcode, uint8_t bytes)
{
  __m128i mout;
  if( bytes == 4 ) {
    mout = _mm_cvtepu8_epi32(shift(utf8, i));
  } else {	
    __m128i Shuf = *(__m128i *) &shuffleTable[bcode[i]][0];
    __m128i bitfields = _mm_shuffle_epi8(shift(utf8, i), Shuf); // masked 
    mout = compress(bitfields);
  }
  _mm_storeu_si128((__m128i *) out, mout);
}

static inline void asciiq( uint32_t *out, __m128i ascii, int i )
{
  _mm_storeu_si128((__m128i *) out, _mm_cvtepu8_epi32(shift(ascii, i)));  
}

static inline int utf32_code_ptr(void *p, uint32_t **pout) {
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

  uint8_t offsets[4];
  uint8_t nbytes[4];
  int nquads=0;
  
  for(i = 0; i < 16 && jmp[i] > i; i = jmp[i])
    {
      int bytes = jmp[i] - i;
      multibyte = multibyte || bytes > 4;
      offsets[nquads]=i;
      nbytes[nquads]=bytes;
      nquads++;
    }

  if(multibyte) {
    __m128i code12 = _mm_or_si128(lengths,_mm_slli_epi64(_mm_shuffle_epi8(lengths, P), 2));  
    __m128i code =   _mm_or_si128(code12,_mm_slli_epi64(_mm_shuffle_epi8(code12, P2), 4));
    uint8_t bcode[16];
    _mm_storeu_si128( (__m128i *)bcode, code );
    utf8 = _mm_and_si128(_mm_shuffle_epi8(mask,upper4), utf8); // mask off in place
    switch(nquads) {
    case 4: storeq((*pout)+12, utf8, offsets[3], bcode, nbytes[3]);
    case 3: storeq((*pout)+8, utf8, offsets[2], bcode, nbytes[2]);
    case 2: storeq((*pout)+4, utf8, offsets[1], bcode, nbytes[1]);
    case 1: storeq(*pout, utf8, offsets[0], bcode, nbytes[0]);
    }
  } else {
    switch(nquads) {
    case 4: asciiq((*pout)+12, utf8, 12);
    case 3: asciiq((*pout)+8, utf8, 8);
    case 2: asciiq((*pout)+4, utf8, 4);
    case 1: asciiq((*pout), utf8, 0);
    }
  }
  *pout += nquads * 4;
  return i;
}

size_t utf32_decode(uint8_t *txt, size_t length, uint32_t **pout)
{
  int n = 0;
  uint8_t *p = txt;
  size_t consumed;
  for(consumed = 0;
      consumed+16 <= length && ((n = utf32_code_ptr(p+consumed, pout))); 
      consumed += n)
    ;

  int l = length - consumed;
  p += consumed;
  
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




