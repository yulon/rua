#ifndef _RUA_TSQUE_HPP
#define _RUA_TSQUE_HPP

#include "optional.hpp"

#include <atomic>
#include <utility>

namespace rua {

template <typename T>
class tsque {
public:
	constexpr tsque() : _root(nullptr) {}

	template <typename... A>
	bool // was empty before
	emplace(A &&... a) {
		auto new_root = new _node_t(std::forward<A>(a)...);
		auto old_root = _root.load();
		do {
			new_root->next = old_root;
		} while (!_root.compare_exchange_weak(old_root, new_root));
		return !old_root;
	}

	optional<T> pop() {
		optional<T> r;
		auto old_root = _root.load();
		if (!old_root) {
			return r;
		}
		while (!_root.compare_exchange_weak(old_root, old_root->next.load()))
			;
		r = optional<T>(std::move(old_root->val));
		delete old_root;
		return r;
	}

private:
	struct _node_t {
		T val;
		std::atomic<_node_t *> next;

		template <typename... A>
		_node_t(A &&... a) : val(std::forward<A>(a)...) {}
	};

	std::atomic<_node_t *> _root;
};

} // namespace rua

#endif
