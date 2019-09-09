#ifndef _RUA_THREAD_THREAD_POSIX_HPP
#define _RUA_THREAD_THREAD_POSIX_HPP

#include "../../any_word.hpp"
#include "../this_thread_id/posix.hpp"
#include "../thread_id/posix.hpp"

#include <pthread.h>

#include <functional>

namespace rua { namespace posix {

class thread {
public:
	using native_handle_t = pthread_t;

	////////////////////////////////////////////////////////////////

	constexpr thread() : _id(0) {}

	explicit thread(std::function<void()> fn) {
		thread_id_t id;
		if (pthread_create(
				&id,
				nullptr,
				[](void *p) -> void * {
					auto f = reinterpret_cast<std::function<void()> *>(p);
					(*f)();
					delete f;
					pthread_detach(id);
					return nullptr;
				},
				reinterpret_cast<void *>(
					new std::function<void()>(std::move(fn))))) {
			return;
			_id = nullptr;
		}
		_id = id;
	}

	constexpr thread(std::nullptr_t) : thread() {}

	explicit thread(thread_id_t id) : _id(id) {}

	~thread() {
		reset();
	}

	thread_id_t id() const {
		return _id;
	}

	bool operator==(const thread &target) const {
		return _id == target._id;
	}

	bool operator!=(const thread &target) const {
		return _id != target._id;
	}

	native_handle_t native_handle() const {
		return _id;
	}

	explicit operator bool() const {
		return _id;
	}

	void exit(any_word retval = nullptr) {
		if (!_id) {
			return;
		}
		if (_id == this_thread_id()) {
			pthread_exit(retval);
		} else {
			pthread_cancel(_id);
		}
		reset();
	}

	any_word wait_for_exit() {
		if (!_id) {
			return nullptr;
		}
		void *retval;
		if (pthread_join(_id, &retval)) {
			return nullptr;
		}
		reset();
		return retval;
	}

	void reset() {
		_id = 0;
	}

private:
	pthread_t _id;
};

}} // namespace rua::posix

#endif
