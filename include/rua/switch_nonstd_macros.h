#ifdef max
#undef max
#define _RUA_UNDEFINED_MAX
#elif defined(_RUA_UNDEFINED_MAX)
#undef _RUA_UNDEFINED_MAX
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifdef min
#undef min
#define _RUA_UNDEFINED_MIN
#elif defined(_RUA_UNDEFINED_MIN)
#undef _RUA_UNDEFINED_MIN
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
