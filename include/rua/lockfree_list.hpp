#ifndef _rua_lockfree_list_hpp
#define _rua_lockfree_list_hpp

#include "forward_list.hpp"
#include "optional.hpp"
#include "util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

template <typename T>
class lockfree_list {
public:
	using node_t = typename forward_list<T>::node_t;

	////////////////////////////////////////////////////////////////////////

	constexpr lockfree_list() : $front(nullptr) {}

	constexpr explicit lockfree_list(node_t *front) : $front(front) {}

	constexpr explicit lockfree_list(forward_list<T> li) :
		$front(li.release()) {}

	lockfree_list(lockfree_list &&src) : $front(src.release()) {}

	lockfree_list &operator=(lockfree_list &&src) {
		if (this == &src) {
			return *this;
		}
		$reset(src.release());
		return *this;
	}

	~lockfree_list() {
		$reset();
	}

	operator bool() const {
		return $front.load();
	}

	bool empty() const {
		return !$front.load();
	}

	template <typename... Args>
	bool emplace_front(Args &&...args) {
		auto new_front = new node_t(std::forward<Args>(args)...);
		auto old_front = $front.load();
		do {
			while (old_front == fullptr) {
				old_front = $front.load();
			}
			new_front->after = old_front;
		} while (!$front.compare_exchange_strong(old_front, new_front));
		return !old_front;
	}

	template <typename Cond, typename... Args>
	bool emplace_front_if(Cond &&cond, Args &&...args) {
		auto li = lock();
		auto r = std::forward<Cond>(cond)();
		if (r) {
			li.emplace_front(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond, typename... Args>
	bool emplace_front_if_non_empty_or(Cond &&cond, Args &&...args) {
		auto li = lock();
		auto r = li || std::forward<Cond>(cond)();
		if (r) {
			li.emplace_front(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename... Args>
	void emplace_back(Args &&...args) {
		auto new_back = new node_t(std::forward<Args>(args)...);
		new_back->after = nullptr;
		auto old_front = $front.load();
		for (;;) {
			while (old_front == fullptr) {
				old_front = $front.load();
			}
			if (!old_front) {
				if ($front.compare_exchange_strong(old_front, new_back)) {
					return;
				}
				continue;
			}
			if ($front.compare_exchange_strong(old_front, fullptr)) {
				break;
			}
		}
		forward_list<T> li(old_front);
		li.push_back_node(new_back);
		unlock_and_prepend(std::move(li));
	}

	template <typename Cond, typename... Args>
	bool emplace_back_if(Cond &&cond, Args &&...args) {
		auto li = lock();
		auto r = std::forward<Cond>(cond)();
		if (r) {
			li.emplace_back(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond, typename... Args>
	bool emplace_back_if_non_empty_or(Cond &&cond, Args &&...args) {
		auto li = lock();
		auto r = !li || std::forward<Cond>(cond)();
		if (r) {
			li.emplace_back(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	bool prepend(forward_list<T> pp) {
		if (!pp) {
			return false;
		}

		auto pp_front = pp.release();
		auto pp_back = pp_front;
		while (pp_back->after) {
			pp_back = pp_back->after;
		}

		auto old_front = $front.load();
		do {
			while (old_front == fullptr) {
				old_front = $front.load();
			}
			pp_back->after = old_front;
		} while (!$front.compare_exchange_strong(old_front, pp_front));
		return !old_front;
	}

	optional<T> pop_front() {
		optional<T> r;
		auto li = lock_if_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_front());
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	optional<T> pop_front_if(Cond &&cond) {
		optional<T> r;
		auto li = lock();
		if (std::forward<Cond>(cond)() && li) {
			r.emplace(li.pop_front());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	optional<T> pop_front_if_non_empty_and(Cond &&cond) {
		optional<T> r;
		auto li = lock_if_non_empty();
		if (!li) {
			return r;
		}
		if (std::forward<Cond>(cond)()) {
			r.emplace(li.pop_front());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename OnEmpty>
	optional<T> pop_front_or(OnEmpty &&on_empty) {
		optional<T> r;
		auto li = lock();
		if (li) {
			r.emplace(li.pop_front());
			unlock_and_prepend(std::move(li));
			return r;
		}
		std::forward<OnEmpty>(on_empty)();
		unlock();
		return r;
	}

	optional<T> pop_back() {
		optional<T> r;
		auto li = lock_if_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_back());
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	optional<T> pop_back_if(Cond &&cond) {
		optional<T> r;
		auto li = lock();
		if (std::forward<Cond>(cond)() && li) {
			r.emplace(li.pop_back());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	optional<T> pop_back_if_non_empty_and(Cond &&cond) {
		optional<T> r;
		auto li = lock_if_non_empty();
		if (!li) {
			return r;
		}
		if (std::forward<Cond>(cond)()) {
			r.emplace(li.pop_back());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	forward_list<T> pop_all() {
		auto front = $front.load();
		do {
			while (front == fullptr) {
				front = $front.load();
			}
		} while (!$front.compare_exchange_strong(front, nullptr));
		return forward_list<T>(front);
	}

	void reset() {
		$reset();
	}

	node_t *release() {
#ifdef NDEBUG
		return $front.exchange(nullptr);
#else
		auto front = $front.exchange(nullptr);
		assert(front == fullptr);
		return front;
#endif
	}

	forward_list<T> lock() {
		auto front = $front.load();
		do {
			while (front == fullptr) {
				front = $front.load();
			}
		} while (!$front.compare_exchange_strong(front, fullptr));
		return forward_list<T>(front);
	}

	forward_list<T> lock_if_non_empty() {
		auto front = $front.load();
		do {
			while (front == fullptr) {
				front = $front.load();
			}
			if (!front) {
				return forward_list<T>();
			}
		} while (!$front.compare_exchange_strong(front, fullptr));
		return forward_list<T>(front);
	}

	void unlock() {
#ifdef NDEBUG
		$front.store(nullptr);
#else
		assert($front.exchange(nullptr) == fullptr);
#endif
	}

	void unlock_and_prepend(forward_list<T> pp) {
		if (!pp) {
			unlock();
			return;
		}
#ifdef NDEBUG
		$front.store(pp.release());
#else
		assert($front.exchange(pp.release()) == fullptr);
#endif
	}

	template <typename... Args>
	void unlock_and_emplace(Args &&...args) {
#ifdef NDEBUG
		$front.store(new node_t(std::forward<Args>(args)...));
#else
		assert(
			$front.exchange(new node_t(std::forward<Args>(args)...)) ==
			fullptr);
#endif
	}

private:
	std::atomic<node_t *> $front;

	void $reset(node_t *new_front = nullptr) {
		auto node = $front.exchange(new_front);
		assert(node != fullptr);
		while (node) {
			auto n = node;
			node = node->after;
			delete n;
		}
	}
};

} // namespace rua

#endif
