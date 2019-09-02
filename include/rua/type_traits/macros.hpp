#ifndef _RUA_TYPE_TRAITS_MACROS_HPP
#define _RUA_TYPE_TRAITS_MACROS_HPP

#include <cstdint>
#include <type_traits>

#define RUA_CONTAINER_OF(member_ptr, type, member)                             \
	reinterpret_cast<type *>(                                                  \
		reinterpret_cast<uintptr_t>(member_ptr) - offsetof(type, member))

#define RUA_IS_BASE_OF_CONCEPT(_B, _D)                                         \
	typename _D, typename = typename std::enable_if<std::is_base_of<           \
					 _B,                                                       \
					 typename std::remove_reference<_D>::type>::value>::type

#define RUA_DERIVED_CONCEPT(_B, _D)                                            \
	typename _D,                                                               \
		typename = typename std::enable_if <                                   \
					   std::is_base_of<                                        \
						   _B,                                                 \
						   typename std::remove_reference<_D>::type>::value && \
				   !std::is_same<_B, _D>::value > ::type

#endif
