#ifndef _RUA_THREAD_LOC_POSIX_HPP
#define _RUA_THREAD_LOC_POSIX_HPP

#include "base.hpp"

#include "../../any_word.hpp"
#include "../../macros.hpp"

#include <pthread.h>

namespace rua { namespace posix {

class thread_loc_word {
public:
	thread_loc_word(void (*dtor)(any_word)) : _dtor(dtor) {
		_invalid = pthread_key_create(
			&_key,
			reinterpret_cast<void (*)(void *)>(reinterpret_cast<void *>(dtor)));
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

	using native_handle_t = pthread_key_t;

	native_handle_t native_handle() const {
		return _key;
	}

	bool is_storable() const {
		return !_invalid;
	}

	void set(any_word value) {
		pthread_setspecific(_key, value);
	}

	any_word get() const {
		return pthread_getspecific(_key);
	}

	void reset() {
		auto val = get();
		if (!val) {
			return;
		}
		_dtor(val);
		set(0);
	}

private:
	pthread_key_t _key;
	bool _invalid;
	void (*_dtor)(any_word);
};

template <typename T>
using thread_loc = basic_thread_loc<T, thread_loc_word>;

}} // namespace rua::posix

#endif
