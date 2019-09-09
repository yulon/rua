#ifndef _RUA_THREAD_THREAD_LOC_UNI_HPP
#define _RUA_THREAD_THREAD_LOC_UNI_HPP

#include "base.hpp"

namespace rua { namespace uni {

using thread_loc_word = spare_thread_loc_word;

template <typename T>
using thread_loc = basic_thread_loc<T, thread_loc_word>;

}} // namespace rua::uni

#endif
