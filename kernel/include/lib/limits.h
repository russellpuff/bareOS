#ifndef H_LIMITS
#define H_LIMITS
#define TYPE_MAX(T) \
    ((T)( ((T)-1 > 0) ? (T)-1 : (T)(((1ULL << (sizeof(T)*8 - 1)) - 1)) ))

#define TYPE_MIN(T) \
    ((T)( ((T)-1 > 0) ? (T)0 : (T)(-((T)(1ULL << (sizeof(T)*8 - 1)))) ))
#endif
