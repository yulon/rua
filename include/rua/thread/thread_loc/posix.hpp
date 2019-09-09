#ifndef _RUA_THREAD_THREAD_LOC_POSIX_HPP
#define _RUA_THREAD_THREAD_LOC_POSIX_HPP

#include "base.hpp"

#include "../../any_word.hpp"
#include "../../macros.hpp"

#include <pthread.h>

namespace rua { namespace posix {

class thread_loc_word {
public:
	thread_loc_word(void (*dtor)(any_word)) {
		_invalid =
			pthread_key_create(&_key, reinterpret_cast<void (*)(void *)>(dtor));
	}

	~thread_loc_word() {
		if (!is_storable()) {
			return;
		}
		if (pthread_key_delete(_key) != 0) {
			return;
		}
		_invalid = true;
	}

	thread_loc_word(thread_loc_word &&src) :
		_key(src._key),
		_invalid(src._invalid) {
		if (src.is_storable()) {
			src._invalid = true;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(thread_loc_word)

	bool is_storable() const {
		return !_invalid;
	}

	bool set(any_word value) {
		return pthread_setspecific(_key, value) == 0;
	}

	any_word get() const {
		return pthread_getspecific(_key);
	}

private:
	pthread_key_t _key;
	bool _invalid;
};

template <typename T>
using thread_loc = basic_thread_loc<T, thread_loc_word>;

}} // namespace rua::posix

#endif
