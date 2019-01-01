void dump(const __m128i x, char * tag) {
  printf( "%6s: ", tag);
  char * xc = (char *) &x;
  for( int i =0; i < 16; i++)
    printf("%2i,", xc[i]);

  printf("\n");
}


void dumpx(const __m128i x, char * tag) {
  printf( "%6s: ", tag);
  unsigned char * xc = (unsigned char *) &x;
  for( int i =0; i < 16; i++)
    printf("%2x,", xc[i]);

  printf("\n");
}
