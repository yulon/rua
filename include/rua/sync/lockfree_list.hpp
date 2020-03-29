#ifndef _RUA_SYNC_LOCKFREE_LIST_HPP
#define _RUA_SYNC_LOCKFREE_LIST_HPP

#include "../forward_list.hpp"
#include "../macros.hpp"

#include <atomic>
#include <cassert>

namespace rua {

template <typename T>
class lockfree_list {
public:
	using node_t = typename forward_list<T>::node_t;

	////////////////////////////////////////////////////////////////////////

	constexpr lockfree_list() : _front(nullptr) {}

	lockfree_list(lockfree_list &&src) : _front(src._front.exchange(nullptr)) {}

	lockfree_list &operator=(lockfree_list &&src) {
		if (this == &src) {
			return *this;
		}
		_reset(src._front.exchange(nullptr));
		return *this;
	}

	~lockfree_list() {
		_reset();
	}

	operator bool() const {
		return _front.load();
	}

	bool empty() const {
		return !_front.load();
	}

	template <typename... Args>
	bool emplace(Args &&... args) {
		return !_push(new node_t(std::forward<Args>(args)...));
	}

	forward_list<T> pop() {
		return forward_list<T>(_front.exchange(nullptr));
	}

	bool prepend(forward_list<T> &&pp) {
		if (!pp) {
			return false;
		}
		auto pp_front = pp.release();
		auto pp_back = pp_front;
		for (;;) {
			auto after = pp_back->after;
			if (!after) {
				break;
			}
			pp_back = after;
		}
		return !_push(pp_front);
	}

	void reset() {
		_reset();
	}

private:
	std::atomic<node_t *> _front;

	node_t *_push(node_t *new_front = nullptr) {
		auto old_front = _front.load();
		do {
			new_front->after = old_front;
		} while (!_front.compare_exchange_weak(old_front, new_front));
		return old_front;
	}

	void _reset(node_t *new_front = nullptr) {
		auto node = _front.exchange(new_front);
		while (node) {
			auto n = node;
			node = node->after;
			delete n;
		}
	}
};

} // namespace rua

#endif
