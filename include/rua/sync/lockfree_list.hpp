#ifndef _RUA_SYNC_LOCKFREE_LIST_HPP
#define _RUA_SYNC_LOCKFREE_LIST_HPP

#include "../macros.hpp"
#include "../opt.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <forward_list>

namespace rua {

/*
	lockfree_list{} is the forward list using atomic operations.

	Access is thread-safe when the following conditions are met:
	1.Use only *_front(). (performance best)
	2.Use only *_front() and erase_with_restore_befores().
	3.Use only emplace_front() and pop_back()/erase_back()/erase(back).
	4.Use only emplace_front() and emplace_back().
	5.Cannot pop_front()/erase(before elements) while iterating.

	Note:
	*_back() and erase() operations are implemented using iteration.
*/
template <typename T>
class lockfree_list {
public:
	constexpr lockfree_list() : _front(nullptr) {}

	lockfree_list(lockfree_list &&src) : _front(src._front.exchange(nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(lockfree_list)

private:
	struct _node_t {
		T val;
		std::atomic<_node_t *> after;

		template <typename... Args>
		_node_t(Args &&... args) : val(std::forward<Args>(args)...) {}
	};

public:
	class iterator {
	public:
		bool is_back() const {
			return !_node->after.load();
		}

		bool operator==(iterator target) const {
			return _node == target._node;
		}

		bool operator!=(iterator target) const {
			return _node != target._node;
		}

		explicit operator bool() const {
			return _node;
		}

		bool operator!() const {
			return !_node;
		}

		iterator &operator++() {
			_node = _node->after.load();
			return *this;
		}

		iterator operator++() const {
			return iterator(_node->after.load());
		}

		iterator operator++(int) {
			auto old = *this;
			_node = _node->after.load();
			return old;
		}

		T &operator*() {
			return _node->val;
		}

		const T &operator*() const {
			return _node->val;
		}

	private:
		friend lockfree_list;
		_node_t *_node;
		constexpr explicit iterator(_node_t *node) : _node(node) {}
	};
	friend iterator;

	template <typename... Args>
	iterator emplace_front(Args &&... args) {
		auto new_front = new _node_t(std::forward<Args>(args)...);
		_insert_node(_front, new_front);
		return iterator(new_front);
	}

	opt<T> pop_front() {
		return _release_node(_pop_node(_front));
	}

	bool erase_with_restore_befores(iterator it) {
		auto ok = false;
		std::forward_list<_node_t *> befores;
		for (;;) {
			auto node = _pop_node(_front);
			if (node == it._node) {
				ok = true;
				delete node;
				break;
			}
			befores.emplace_front(node);
		}
		while (!befores.empty()) {
			_insert_node(_front, befores.front());
			befores.pop_front();
		}
		return ok;
	}

	bool is_empty() const {
		return !_front.load();
	}

	iterator begin() {
		return iterator(_front.load());
	}

	constexpr iterator end() const {
		return iterator(nullptr);
	}

	template <typename... Args>
	iterator emplace_back(Args &&... args) {
		auto new_back = new _node_t(std::forward<Args>(args)...);
		new_back->after = nullptr;
		_insert_back_node(&_front);
		return iterator(new_back);
	}

	opt<T> pop_back() {
		_node_t *before, *old_back;
		_pop_back_node(before, old_back);
		if (old_back) {
			return _release_node(old_back);
		}
		return nullopt;
	}

	opt<iterator> erase_back() {
		_node_t *before, *old_back;
		_pop_back_node(before, old_back);
		if (old_back) {
			delete old_back;
			return make_opt(iterator(before));
		}
		return nullopt;
	}

	opt<iterator> erase(iterator it) {
		_node_t *before = nullptr, *n = _front.load();
		while (n) {
			while (n != it._node) {
				auto after = n->after.load();
				if (!after) {
					return nullopt;
					;
				}
				before = n;
				n = after;
			}
			auto &slot = before ? before->after : _front;
			auto slot_val = n;
			while (!slot.compare_exchange_weak(slot_val, n->after.load()) &&
				   slot_val == n)
				;
			if (slot_val == n) {
				delete n;
				return make_opt(iterator(before));
			}
			if (!slot_val) {
				return nullopt;
				;
			}
			n = slot_val;
		}
		return nullopt;
		;
	}

private:
	std::atomic<_node_t *> _front;

	static void _insert_node(std::atomic<_node_t *> &slot, _node_t *new_node) {
		auto old_node = slot.load();
		do {
			new_node->after = old_node;
		} while (!slot.compare_exchange_weak(old_node, new_node));
	}

	static void
	_insert_back_node(std::atomic<_node_t *> *slot, _node_t *new_node) {
		_node_t *slot_val = nullptr;
		while (!slot->compare_exchange_weak(slot_val, new_node)) {
			if (!slot_val) {
				continue;
			}
			slot_val = nullptr;
			slot = &slot->load()->after;
		}
	}

	static _node_t *_pop_node(std::atomic<_node_t *> &slot) {
		auto old_node = slot.load();
		if (!old_node) {
			return nullptr;
		}
		while (!slot.compare_exchange_weak(old_node, old_node->after.load()) &&
			   old_node)
			;
		return old_node;
	}

	void _pop_back_node(_node_t *&before, _node_t *&old_back) {
		before = nullptr;
		old_back = _front.load();
		while (old_back) {
			for (;;) {
				auto after = old_back->after.load();
				if (!after) {
					break;
				}
				before = old_back;
				old_back = after;
			}
			auto &slot = before ? before->after : _front;
			auto slot_val = old_back;
			while (!slot.compare_exchange_weak(slot_val, nullptr) &&
				   slot_val == old_back)
				;
			if (slot_val == old_back) {
				return;
			}
			if (!slot_val) {
				before = nullptr;
				old_back = _front.load();
			} else {
				old_back = slot_val;
			}
		}
	}

	static opt<T> _release_node(_node_t *node) {
		opt<T> r;
		if (!node) {
			return r;
		}
		r = opt<T>(std::move(node->val));
		delete node;
		return r;
	}
};

} // namespace rua

#endif