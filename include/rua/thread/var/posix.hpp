#ifndef _rua_thread_var_posix_hpp
#define _rua_thread_var_posix_hpp

#include "base.hpp"

#include "../../generic_word.hpp"
#include "../../util.hpp"

#include <pthread.h>

namespace rua { namespace posix {

class thread_word_var {
public:
	thread_word_var(void (*dtor)(generic_word)) : $dtor(dtor) {
		$invalid = pthread_key_create(
			&$key,
			reinterpret_cast<void (*)(void *)>(reinterpret_cast<void *>(dtor)));
	}

	~thread_word_var() {
		if (!is_storable()) {
			return;
		}
		if (pthread_key_delete($key) != 0) {
			return;
		}
		$invalid = true;
	}

	thread_word_var(thread_word_var &&src) :
		$key(src.$key), $invalid(src.$invalid) {
		if (src.is_storable()) {
			src.$invalid = true;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(thread_word_var)

	using native_handle_t = pthread_key_t;

	native_handle_t native_handle() const {
		return $key;
	}

	bool is_storable() const {
		return !$invalid;
	}

	void set(generic_word value) {
		pthread_setspecific($key, value);
	}

	generic_word get() const {
		return pthread_getspecific($key);
	}

	void reset() {
		auto val = get();
		if (!val) {
			return;
		}
		$dtor(val);
		set(0);
	}

private:
	pthread_key_t $key;
	bool $invalid;
	void (*$dtor)(generic_word);
};

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::posix

#endif
