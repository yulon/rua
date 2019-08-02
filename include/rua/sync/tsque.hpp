#ifndef _RUA_TSQUE_HPP
#define _RUA_TSQUE_HPP

#include "../macros.hpp"
#include "../optional.hpp"

#include <atomic>
#include <forward_list>
#include <utility>

namespace rua {

template <typename T>
class tsque {
public:
	constexpr tsque() : _root(nullptr) {}

	tsque(tsque &&src) : _root(src._root.exchange(nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(tsque)

	class noder;

	template <typename... A>
	noder emplace(A &&... a) {
		auto new_root = new _node_t(std::forward<A>(a)...);
		_push_node(_root, new_root);
		return noder(new_root);
	}

	optional<T> pop() {
		return _release_node(_pop_node(_root));
	}

	// If used with emplace(), the result of noder::is_first() may be
	// inaccurate.
	bool erase(noder n) {
		auto ok = false;
		std::forward_list<_node_t *> befores;
		for (;;) {
			auto node = _pop_node(_root);
			if (node == n._node) {
				ok = true;
				break;
			}
			befores.emplace_front(node);
		}
		while (!befores.empty()) {
			_push_node(_root, befores.front());
			befores.pop_front();
		}
		return ok;
	}

private:
	struct _node_t {
		T val;
		std::atomic<_node_t *> before;

		template <typename... A>
		_node_t(A &&... a) : val(std::forward<A>(a)...) {}
	};

	std::atomic<_node_t *> _root;

	static void _push_node(std::atomic<_node_t *> &slot, _node_t *new_node) {
		auto old_node = slot.load();
		do {
			new_node->before = old_node;
		} while (!slot.compare_exchange_weak(old_node, new_node));
	}

	static _node_t *_pop_node(std::atomic<_node_t *> &slot) {
		auto old_node = slot.load();
		if (!old_node) {
			return nullptr;
		}
		while (!slot.compare_exchange_weak(old_node, old_node->before.load()))
			;
		return old_node;
	}

	static optional<T> _release_node(_node_t *node) {
		optional<T> r;
		if (!node) {
			return r;
		}
		r = optional<T>(std::move(node->val));
		delete node;
		return r;
	}

public:
	class noder {
	public:
		constexpr explicit noder(_node_t *node) : _node(node) {}

		bool is_first() const {
			return !_node->before.load();
		}

		bool operator==(noder target) const {
			return _node == target._node;
		}

		bool operator!=(noder target) const {
			return _node != target._node;
		}

		explicit operator bool() const {
			return _node;
		}

		bool operator!() const {
			return !_node;
		}

	private:
		friend tsque;
		_node_t *_node;
	};

private:
	friend noder;
};

} // namespace rua

#endif
