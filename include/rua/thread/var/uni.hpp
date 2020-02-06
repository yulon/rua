#ifndef _RUA_THREAD_VAR_UNI_HPP
#define _RUA_THREAD_VAR_UNI_HPP

#include "base.hpp"

namespace rua { namespace uni {

using thread_word_var = spare_thread_word_var;

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::uni

#endif
