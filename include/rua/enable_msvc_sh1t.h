#ifdef _MSC_VER
	#ifdef _RUA_HAS_MAX
		#undef _RUA_HAS_MAX
		#define max(a,b) (((a) > (b)) ? (a) : (b))
	#endif

	#ifdef _RUA_HAS_MIN
		#undef _RUA_HAS_MIN
		#define min(a,b) (((a) < (b)) ? (a) : (b))
	#endif
#endif
