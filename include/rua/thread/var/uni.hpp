#ifndef _rua_thread_var_uni_hpp
#define _rua_thread_var_uni_hpp

#include "base.hpp"

namespace rua { namespace uni {

using thread_word_var = spare_thread_word_var;

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::uni

#endif
