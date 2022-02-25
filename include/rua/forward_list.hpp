#ifndef _RUA_FORWARD_LIST_HPP
#define _RUA_FORWARD_LIST_HPP

#include "macros.hpp"
#include "types/util.hpp"

#include <cassert>

namespace rua {

template <typename T>
class forward_list {
public:
	struct node_t {
		T value;
		node_t *after;

		template <typename... Args>
		node_t(Args &&...args) : value(std::forward<Args>(args)...) {}
	};

	class const_iterator {
	public:
		constexpr const_iterator() : _n(nullptr) {}

		constexpr explicit const_iterator(node_t *n) : _n(n) {}

		bool operator==(const_iterator target) const {
			return _n == target._n;
		}

		bool operator!=(const_iterator target) const {
			return _n != target._n;
		}

		operator bool() const {
			return _n;
		}

		const_iterator operator++() const {
			return const_iterator(_n->after);
		}

		const T &operator*() const {
			return _n->value;
		}

		node_t *node() const {
			return _n;
		}

	protected:
		node_t *_n;
	};

	class iterator : public const_iterator {
	public:
		constexpr iterator() : const_iterator() {}

		constexpr explicit iterator(node_t *n) : const_iterator(n) {}

		iterator &operator++() {
			this->_n = this->_n->after;
			return *this;
		}

		iterator operator++() const {
			return iterator(this->_n->after);
		}

		iterator operator++(int) {
			auto old = *this;
			this->_n = this->_n->after;
			return old;
		}

		T &operator*() {
			return this->_n->value;
		}

		const T &operator*() const {
			return this->_n->value;
		}
	};

	////////////////////////////////////////////////////////////////////////

	constexpr forward_list() : _front(nullptr) {}

	constexpr explicit forward_list(node_t *front) : _front(front) {}

	forward_list(const forward_list &src) : _front(nullptr) {
		if (!src._front) {
			_front = nullptr;
			return;
		}
		auto src_it = src.begin();
		auto it = emplace_front(*(src_it++));
		for (; src_it != src.end(); ++src_it) {
			it = emplace_after(*src_it);
		}
	}

	forward_list(forward_list &&src) : _front(src._front) {
		if (src._front) {
			src._front = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(forward_list)

	~forward_list() {
		reset();
	}

	operator bool() const {
		return _front;
	}

	bool empty() const {
		return !_front;
	}

	template <typename... Args>
	iterator emplace_front(Args &&...args) {
		auto new_front = new node_t(std::forward<Args>(args)...);
		push_front_node(new_front);
		return iterator(new_front);
	}

	void push_front_node(node_t *new_front_node) {
		new_front_node->after = _front;
		_front = new_front_node;
	}

	template <typename... Args>
	iterator emplace_back(Args &&...args) {
		auto new_back = new node_t(std::forward<Args>(args)...);
		push_back_node(new_back);
		return iterator(new_back);
	}

	void push_back_node(node_t *new_back_node) {
		auto slot = &_front;
		while (*slot) {
			slot = &(*slot)->after;
		}
		*slot = new_back_node;
	}

	template <typename... Args>
	iterator emplace_after(const_iterator before, Args &&...args) {
		assert(before);

		auto new_after = new node_t(std::forward<Args>(args)...);
		insert_after_node(before.node(), new_after);
		return iterator(new_after);
	}

	void insert_after_node(node_t *before_node, node_t *new_after_node) {
		new_after_node->after = before_node->after;
		before_node->after = new_after_node;
	}

	T pop_front() {
		return _pop(_front);
	}

	T pop_back() {
		return _pop(_back());
	}

	T pop_after(const_iterator before) {
		assert(_front);
		assert(before.node());

		return _pop(before.node()->after);
	}

	iterator erase_front() {
		return _erase(_front);
	}

	void erase_back() {
		assert(_front);

		auto &back = _back();
		delete back;
		back = nullptr;
	}

	iterator erase_after(const_iterator before) {
		assert(_front);
		assert(before.node());

		return _erase(before.node()->after);
	}

	T &front() {
		assert(_front);

		return _front->value;
	}

	const T &front() const {
		assert(_front);

		return _front->value;
	}

	iterator begin() {
		return iterator(_front);
	}

	const_iterator begin() const {
		return const_iterator(_front);
	}

	RUA_CONSTEXPR_14 iterator end() {
		return iterator();
	}

	constexpr const_iterator end() const {
		return const_iterator();
	}

	void reset() {
		auto n = _front;
		while (n) {
			auto after = n->after;
			delete n;
			n = after;
		}
	}

	node_t *release() {
		auto front = _front;
		_front = nullptr;
		return front;
	}

private:
	node_t *_front;

	T _pop(node_t *&n) {
		assert(n);

		auto after = n->after;
		T r(std::move(n->value));
		delete n;
		n = after;
		return r;
	}

	iterator _erase(node_t *&n) {
		assert(n);

		iterator it(n->after);
		delete n;
		n = it.node();
		return it;
	}

	node_t *&_back() {
		assert(_front);

		auto slot = &_front;
		while ((*slot)->after) {
			slot = &(*slot)->after;
		}
		return *slot;
	}
};

} // namespace rua

#endif
