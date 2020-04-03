#ifndef _RUA_SYNC_LOCKFREE_LIST_HPP
#define _RUA_SYNC_LOCKFREE_LIST_HPP

#include "../forward_list.hpp"
#include "../macros.hpp"
#include "../opt.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

template <typename T>
class lockfree_list {
public:
	using node_t = typename forward_list<T>::node_t;

	////////////////////////////////////////////////////////////////////////

	constexpr lockfree_list() : _front(nullptr) {}

	constexpr explicit lockfree_list(node_t *front) : _front(nullptr) {}

	constexpr explicit lockfree_list(forward_list<T> li) :
		_front(li.release()) {}

	lockfree_list(lockfree_list &&src) : _front(src.release()) {}

	lockfree_list &operator=(lockfree_list &&src) {
		if (this == &src) {
			return *this;
		}
		_reset(src.release());
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
	bool emplace_front(Args &&... args) {
		auto new_front = new node_t(std::forward<Args>(args)...);
		auto old_front = _front.load();
		do {
			while (old_front == reinterpret_cast<node_t *>(_locked)) {
				old_front = _front.load();
			}
			new_front->after = old_front;
		} while (!_front.compare_exchange_weak(old_front, new_front));
		return !old_front;
	}

	template <typename Cond, typename... Args>
	bool emplace_front_if(Cond &&cond, Args &&... args) {
		auto li = lock();
		auto r = cond();
		if (r) {
			li.emplace_front(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond, typename... Args>
	bool emplace_front_if_non_empty_or(Cond &&cond, Args &&... args) {
		auto li = lock();
		auto r = !li || cond();
		if (r) {
			li.emplace_front(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename... Args>
	opt<bool> try_emplace_front(Args &&... args) {
		node_t *new_front = nullptr;
		auto old_front = _front.load();
		do {
			if (old_front == reinterpret_cast<node_t *>(_locked)) {
				if (new_front) {
					delete new_front;
				}
				return nullopt;
			}
			if (!new_front) {
				new_front = new node_t(std::forward<Args>(args)...);
			}
			new_front->after = old_front;
		} while (!_front.compare_exchange_weak(old_front, new_front));
		return !old_front;
	}

	template <typename... Args>
	void emplace_back(Args &&... args) {
		auto new_back = new node_t(std::forward<Args>(args)...);
		new_back->after = nullptr;
		auto old_front = _front.load();
		for (;;) {
			while (old_front == reinterpret_cast<node_t *>(_locked)) {
				old_front = _front.load();
			}
			if (!old_front) {
				if (_front.compare_exchange_weak(old_front, new_back)) {
					return;
				}
				continue;
			}
			if (_front.compare_exchange_weak(
					old_front, reinterpret_cast<node_t *>(_locked))) {
				break;
			}
		}
		forward_list<T> li(old_front);
		li.push_back_node(new_back);
		unlock_and_prepend(std::move(li));
	}

	template <typename Cond, typename... Args>
	bool emplace_back_if(Cond &&cond, Args &&... args) {
		auto li = lock();
		auto r = cond();
		if (r) {
			li.emplace_back(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond, typename... Args>
	bool emplace_back_if_non_empty_or(Cond &&cond, Args &&... args) {
		auto li = lock();
		auto r = !li || cond();
		if (r) {
			li.emplace_back(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename... Args>
	bool try_emplace_back(Args &&... args) {
		node_t *new_back = nullptr;
		auto old_front = _front.load();
		for (;;) {
			if (old_front == reinterpret_cast<node_t *>(_locked)) {
				if (new_back) {
					delete new_back;
				}
				return false;
			}
			if (!old_front) {
				if (!new_back) {
					new_back = new node_t(std::forward<Args>(args)...);
					new_back->after = nullptr;
				}
				if (_front.compare_exchange_weak(old_front, new_back)) {
					return true;
				}
				continue;
			}
			if (_front.compare_exchange_weak(
					old_front, reinterpret_cast<node_t *>(_locked))) {
				break;
			}
		}
		forward_list<T> li(old_front);
		if (new_back) {
			li.push_back_node(new_back);
		} else {
			li.emplace_back(std::forward<Args>(args)...);
		}
		unlock_and_prepend(std::move(li));
		return true;
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

		auto old_front = _front.load();
		do {
			while (old_front == reinterpret_cast<node_t *>(_locked)) {
				old_front = _front.load();
			}
			pp_back->after = old_front;
		} while (!_front.compare_exchange_weak(old_front, pp_front));
		return !old_front;
	}

	opt<bool> try_prepend(forward_list<T> pp) {
		if (!pp) {
			return false;
		}

		auto pp_front = pp.release();
		auto pp_back = pp_front;
		while (pp_back->after) {
			pp_back = pp_back->after;
		}

		auto old_front = _front.load();
		do {
			if (old_front == reinterpret_cast<node_t *>(_locked)) {
				return nullopt;
			}
			pp_back->after = old_front;
		} while (!_front.compare_exchange_weak(old_front, pp_front));
		return !old_front;
	}

	opt<T> pop_front() {
		opt<T> r;
		auto li = lock_when_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_front());
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	opt<T> pop_front_if(Cond &&cond) {
		opt<T> r;
		auto li = lock_when_non_empty();
		if (!li) {
			return r;
		}
		if (cond()) {
			r.emplace(li.pop_front());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	opt<T> try_pop_front() {
		opt<T> r;
		auto li = try_lock_when_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_front());
		unlock_and_prepend(std::move(li));
		return r;
	}

	opt<T> pop_back() {
		opt<T> r;
		auto li = lock_when_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_back());
		unlock_and_prepend(std::move(li));
		return r;
	}

	template <typename Cond>
	opt<T> pop_back_if(Cond &&cond) {
		opt<T> r;
		auto li = lock_when_non_empty();
		if (!li) {
			return r;
		}
		if (cond()) {
			r.emplace(li.pop_back());
		}
		unlock_and_prepend(std::move(li));
		return r;
	}

	opt<T> try_pop_back() {
		opt<T> r;
		auto li = try_lock_when_non_empty();
		if (!li) {
			return r;
		}
		r.emplace(li.pop_back());
		unlock_and_prepend(std::move(li));
		return r;
	}

	forward_list<T> pop_all() {
		auto front = _front.load();
		do {
			while (front == reinterpret_cast<node_t *>(_locked)) {
				front = _front.load();
			}
		} while (!_front.compare_exchange_weak(front, nullptr));
		return forward_list<T>(front);
	}

	opt<forward_list<T>> try_pop_all() {
		auto front = _front.load();
		do {
			if (front == reinterpret_cast<node_t *>(_locked)) {
				return nullopt;
			}
		} while (!_front.compare_exchange_weak(front, nullptr));
		return forward_list<T>(front);
	}

	forward_list<T> lock() {
		auto front = _front.load();
		do {
			while (front == reinterpret_cast<node_t *>(_locked)) {
				front = _front.load();
			}
		} while (!_front.compare_exchange_weak(
			front, reinterpret_cast<node_t *>(_locked)));
		return forward_list<T>(front);
	}

	opt<forward_list<T>> try_lock() {
		auto front = _front.exchange(reinterpret_cast<node_t *>(_locked));
		if (front == reinterpret_cast<node_t *>(_locked)) {
			return nullopt;
		}
		return forward_list<T>(front);
	}

	forward_list<T> lock_when_non_empty() {
		auto front = _front.load();
		do {
			while (front == reinterpret_cast<node_t *>(_locked)) {
				front = _front.load();
			}
			if (!front) {
				return forward_list<T>();
			}
		} while (!_front.compare_exchange_weak(
			front, reinterpret_cast<node_t *>(_locked)));
		return forward_list<T>(front);
	}

	forward_list<T> try_lock_when_non_empty() {
		auto front = _front.load();
		do {
			if (!front || front == reinterpret_cast<node_t *>(_locked)) {
				return forward_list<T>();
			}
		} while (!_front.compare_exchange_weak(
			front, reinterpret_cast<node_t *>(_locked)));
		return forward_list<T>(front);
	}

	void unlock() {
#ifdef NDEBUG
		_front.store(nullptr);
#else
		assert(_front.exchange(nullptr) == reinterpret_cast<node_t *>(_locked));
#endif
	}

	void unlock_and_prepend(forward_list<T> pp) {
		if (!pp) {
			unlock();
			return;
		}
#ifdef NDEBUG
		_front.store(pp.release());
#else
		assert(
			_front.exchange(pp.release()) ==
			reinterpret_cast<node_t *>(_locked));
#endif
	}

	template <typename... Args>
	void unlock_and_emplace(Args &&... args) {
#ifdef NDEBUG
		_front.store(new node_t(std::forward<Args>(args)...));
#else
		assert(
			_front.exchange(new node_t(std::forward<Args>(args)...)) ==
			reinterpret_cast<node_t *>(_locked));
#endif
	}

	void reset() {
		_reset();
	}

	node_t *release() {
#ifdef NDEBUG
		return _front.exchange(nullptr);
#else
		auto front = _front.exchange(nullptr);
		assert(front == reinterpret_cast<node_t *>(_locked));
		return front;
#endif
	}

private:
	std::atomic<node_t *> _front;

	static constexpr auto _locked = nmax<uintptr_t>();

	void _reset(node_t *new_front = nullptr) {
		auto node = _front.exchange(new_front);
		assert(node != reinterpret_cast<node_t *>(_locked));
		while (node) {
			auto n = node;
			node = node->after;
			delete n;
		}
	}
};

} // namespace rua

#endif
