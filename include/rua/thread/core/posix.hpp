#ifndef _rua_thread_core_posix_hpp
#define _rua_thread_core_posix_hpp

#include "../id/posix.hpp"

#include "../../generic_word.hpp"
#include "../../sync/await.hpp"
#include "../../sync/future.hpp"
#include "../../util.hpp"

#include <pthread.h>

#include <functional>
#include <memory>

namespace rua { namespace posix {

class thread : private enable_await_operators {
public:
	using native_handle_t = pthread_t;

	////////////////////////////////////////////////////////////////

	constexpr thread() : $id(0), $res() {}

	explicit thread(std::function<void()> fn, size_t stack_size = 0) {
		$res = std::make_shared<$res_t>();
		if (pthread_attr_init(&$res->attr)) {
			$res->id = 0;
			$res.reset();
			$id = 0;
			return;
		}
		if (stack_size && pthread_attr_setstacksize(&$res->attr, stack_size)) {
			pthread_attr_destroy(&$res->attr);
			$res->id = 0;
			$res.reset();
			$id = 0;
			return;
		}
		$res->fn = std::move(fn);
		if (pthread_create(
				&$res->id,
				&$res->attr,
				[](void *p) -> void * {
					std::unique_ptr<std::shared_ptr<$res_t>> res_ptr(
						reinterpret_cast<std::shared_ptr<$res_t> *>(p));
					(*res_ptr)->fn();
					return nullptr;
				},
				reinterpret_cast<void *>(new std::shared_ptr<$res_t>($res)))) {
			pthread_attr_destroy(&$res->attr);
			$res->id = 0;
			$res.reset();
			$id = 0;
			return;
		}
		$id = $res->id;
	}

	constexpr thread(std::nullptr_t) : thread() {}

	constexpr explicit thread(tid_t id) : $id(id), $res() {}

	~thread() {
		reset();
	}

	tid_t id() const {
		return $id;
	}

	native_handle_t native_handle() const {
		return $id;
	}

	explicit operator bool() const {
		return $id;
	}

	bool operator==(const thread &target) const {
		return $id == target.$id;
	}

	bool operator!=(const thread &target) const {
		return $id != target.$id;
	}

	void exit(generic_word retval = nullptr) {
		if (!$id) {
			return;
		}
		if ($id == this_tid()) {
			pthread_exit(retval);
		} else {
			pthread_cancel($id);
		}
		reset();
	}

	inline future<generic_word> RUA_OPERATOR_AWAIT() const;

	void reset() {
		if ($id) {
			$id = 0;
		}
		$res.reset();
	}

private:
	pthread_t $id;

	struct $res_t {
		pthread_t id;
		pthread_attr_t attr;
		std::function<void()> fn;

		$res_t() = default;

		~$res_t() {
			if (!id) {
				return;
			}
			pthread_detach(id);
			pthread_attr_destroy(&attr);
		}

		$res_t($res_t &&src) :
			id(src.id), attr(src.attr), fn(std::move(src.fn)) {
			if (src.id) {
				src.id = 0;
			}
		}

		RUA_OVERLOAD_ASSIGNMENT($res_t)
	};

	std::shared_ptr<$res_t> $res;
};

namespace _this_thread {

inline thread this_thread() {
	return thread(this_tid());
}

} // namespace _this_thread

using namespace _this_thread;

}} // namespace rua::posix

#endif
