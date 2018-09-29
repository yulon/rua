#ifndef _RUA_STLDATA_HPP
#define _RUA_STLDATA_HPP

namespace rua {
	template <typename T>
	inline typename T::value_type *stldata(T &target) {
		#if RUA_CPP >= RUA_CPP_17
			return target.data();
		#else
			return const_cast<typename T::value_type *>(target.data());
		#endif
	}
}

#endif
