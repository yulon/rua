#ifndef _RUA_THREAD_VAR_POSIX_HPP
#define _RUA_THREAD_VAR_POSIX_HPP

#include "base.hpp"

#include "../../generic_word.hpp"
#include "../../util.hpp"

#include <pthread.h>

namespace rua { namespace posix {

class thread_word_var {
public:
	thread_word_var(void (*dtor)(generic_word)) : _dtor(dtor) {
		_invalid = pthread_key_create(
			&_key,
			reinterpret_cast<void (*)(void *)>(reinterpret_cast<void *>(dtor)));
	}

	~thread_word_var() {
		if (!is_storable()) {
			return;
		}
		if (pthread_key_delete(_key) != 0) {
			return;
		}
		_invalid = true;
	}

	thread_word_var(thread_word_var &&src) :
		_key(src._key), _invalid(src._invalid) {
		if (src.is_storable()) {
			src._invalid = true;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(thread_word_var)

	using native_handle_t = pthread_key_t;

	native_handle_t native_handle() const {
		return _key;
	}

	bool is_storable() const {
		return !_invalid;
	}

	void set(generic_word value) {
		pthread_setspecific(_key, value);
	}

	generic_word get() const {
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
	void (*_dtor)(generic_word);
};

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::posix

#endif
