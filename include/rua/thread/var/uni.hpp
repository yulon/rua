#ifndef _RUA_THREAD_VAR_UNI_HPP
#define _RUA_THREAD_VAR_UNI_HPP

#include "base.hpp"

namespace rua { namespace uni {

using thread_storage = spare_thread_storage;

template <typename T>
using thread_var = basic_thread_var<T, thread_storage>;

}} // namespace rua::uni

#endif
