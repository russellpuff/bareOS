
void* memset(void* s, int c, int n) {
  while (--n >= 0) ((char*)s)[n] = (char)c;
  return s;
}


void* memcpy(void* dst, const void* src, int n) {
  while (--n >= 0) ((char*)dst)[n] = ((char*)src)[n];
  return dst;
}
