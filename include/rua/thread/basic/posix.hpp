#ifndef _RUA_THREAD_BASIC_POSIX_HPP
#define _RUA_THREAD_BASIC_POSIX_HPP

#include "../../generic_word.hpp"
#include "../../macros.hpp"

#include <pthread.h>

#include <functional>
#include <memory>

namespace rua { namespace posix {

using tid_t = pthread_t;

namespace _this_tid {

inline tid_t this_tid() {
	return pthread_self();
}

} // namespace _this_tid

using namespace _this_tid;

class thread {
public:
	using native_handle_t = pthread_t;

	////////////////////////////////////////////////////////////////

	constexpr thread() : _id(0), _res() {}

	explicit thread(std::function<void()> fn, size_t stack_size = 0) {
		_res = std::make_shared<_res_t>();
		if (pthread_attr_init(&_res->attr)) {
			_res->id = 0;
			_res.reset();
			_id = 0;
			return;
		}
		if (stack_size && pthread_attr_setstacksize(&_res->attr, stack_size)) {
			pthread_attr_destroy(&_res->attr);
			_res->id = 0;
			_res.reset();
			_id = 0;
			return;
		}
		_res->fn = std::move(fn);
		if (pthread_create(
				&_res->id,
				&_res->attr,
				[](void *p) -> void * {
					std::unique_ptr<std::shared_ptr<_res_t>> res_ptr(
						reinterpret_cast<std::shared_ptr<_res_t> *>(p));
					(*res_ptr)->fn();
					return nullptr;
				},
				reinterpret_cast<void *>(new std::shared_ptr<_res_t>(_res)))) {
			pthread_attr_destroy(&_res->attr);
			_res->id = 0;
			_res.reset();
			_id = 0;
			return;
		}
		_id = _res->id;
	}

	constexpr thread(std::nullptr_t) : thread() {}

	constexpr explicit thread(tid_t id) : _id(id), _res() {}

	~thread() {
		reset();
	}

	tid_t id() const {
		return _id;
	}

	native_handle_t native_handle() const {
		return _id;
	}

	explicit operator bool() const {
		return _id;
	}

	bool operator==(const thread &target) const {
		return _id == target._id;
	}

	bool operator!=(const thread &target) const {
		return _id != target._id;
	}

	void exit(generic_word retval = nullptr) {
		if (!_id) {
			return;
		}
		if (_id == this_tid()) {
			pthread_exit(retval);
		} else {
			pthread_cancel(_id);
		}
		reset();
	}

	inline generic_word wait();

	void reset() {
		if (_id) {
			_id = 0;
		}
		_res.reset();
	}

private:
	pthread_t _id;

	struct _res_t {
		pthread_t id;
		pthread_attr_t attr;
		std::function<void()> fn;

		_res_t() = default;

		~_res_t() {
			if (!id) {
				return;
			}
			pthread_detach(id);
			pthread_attr_destroy(&attr);
		}

		_res_t(_res_t &&src) :
			id(src.id), attr(src.attr), fn(std::move(src.fn)) {
			if (src.id) {
				src.id = 0;
			}
		}

		RUA_OVERLOAD_ASSIGNMENT_R(_res_t)
	};

	std::shared_ptr<_res_t> _res;
};

namespace _this_thread {

inline thread this_thread() {
	return thread(this_tid());
}

} // namespace _this_thread

using namespace _this_thread;

}} // namespace rua::posix

#endif
