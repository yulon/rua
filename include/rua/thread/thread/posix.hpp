#ifndef _RUA_THREAD_THREAD_POSIX_HPP
#define _RUA_THREAD_THREAD_POSIX_HPP

#include "../this_thread_id/posix.hpp"
#include "../thread_id/posix.hpp"

#include "../../any_word.hpp"

#include <pthread.h>

#include <functional>
#include <memory>

namespace rua { namespace posix {

class thread {
public:
	using native_handle_t = pthread_t;

	////////////////////////////////////////////////////////////////

	constexpr thread() : _id(0), _d() {}

	explicit thread(std::function<void()> fn) {
		tid_t id;
		if (pthread_create(
				&id,
				nullptr,
				[](void *p) -> void * {
					std::unique_ptr<std::function<void()>> fp(
						reinterpret_cast<std::function<void()> *>(p));
					(*fp)();
					return nullptr;
				},
				reinterpret_cast<void *>(
					new std::function<void()>(std::move(fn))))) {
			_id = 0;
			return;
		}
		_id = id;
		_d = std::make_shared<_detacher_t>(id);
	}

	constexpr thread(std::nullptr_t) : thread() {}

	constexpr explicit thread(tid_t id) : _id(id), _d() {}

	~thread() {
		reset();
	}

	tid_t id() const {
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
		_d.reset();
	}

private:
	pthread_t _id;

	class _detacher_t {
	public:
		constexpr _detacher_t(pthread_t id) : _id(id) {}

		~_detacher_t() {
			pthread_detach(_id);
		}

	private:
		pthread_t _id;
	};

	std::shared_ptr<_detacher_t> _d;
};

}} // namespace rua::posix

#endif
