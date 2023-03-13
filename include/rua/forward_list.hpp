#ifndef _rua_forward_list_hpp
#define _rua_forward_list_hpp

#include "util.hpp"

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
		constexpr const_iterator() : $n(nullptr) {}

		constexpr explicit const_iterator(node_t *n) : $n(n) {}

		bool operator==(const_iterator target) const {
			return $n == target.$n;
		}

		bool operator!=(const_iterator target) const {
			return $n != target.$n;
		}

		operator bool() const {
			return $n;
		}

		const_iterator operator++() const {
			return const_iterator($n->after);
		}

		const T &operator*() const {
			return $n->value;
		}

		node_t *node() const {
			return $n;
		}

	protected:
		node_t *$n;
	};

	class iterator : public const_iterator {
	public:
		constexpr iterator() : const_iterator() {}

		constexpr explicit iterator(node_t *n) : const_iterator(n) {}

		iterator &operator++() {
			this->$n = this->$n->after;
			return *this;
		}

		iterator operator++() const {
			return iterator(this->$n->after);
		}

		iterator operator++(int) {
			auto old = *this;
			this->$n = this->$n->after;
			return old;
		}

		T &operator*() {
			return this->$n->value;
		}

		const T &operator*() const {
			return this->$n->value;
		}
	};

	////////////////////////////////////////////////////////////////////////

	constexpr forward_list() : $front(nullptr) {}

	constexpr explicit forward_list(node_t *front) : $front(front) {}

	forward_list(const forward_list &src) : $front(nullptr) {
		if (!src.$front) {
			$front = nullptr;
			return;
		}
		auto src_it = src.begin();
		auto it = emplace_front(*(src_it++));
		for (; src_it != src.end(); ++src_it) {
			it = emplace_after(*src_it);
		}
	}

	forward_list(forward_list &&src) : $front(src.$front) {
		if (src.$front) {
			src.$front = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(forward_list)

	~forward_list() {
		reset();
	}

	operator bool() const {
		return $front;
	}

	bool empty() const {
		return !$front;
	}

	template <typename... Args>
	iterator emplace_front(Args &&...args) {
		auto new_front = new node_t(std::forward<Args>(args)...);
		push_front_node(new_front);
		return iterator(new_front);
	}

	void push_front_node(node_t *new_front_node) {
		new_front_node->after = $front;
		$front = new_front_node;
	}

	template <typename... Args>
	iterator emplace_back(Args &&...args) {
		auto new_back = new node_t(std::forward<Args>(args)...);
		push_back_node(new_back);
		return iterator(new_back);
	}

	void push_back_node(node_t *new_back_node) {
		auto slot = &$front;
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
		return $pop($front);
	}

	T pop_back() {
		return $pop($back());
	}

	T pop_after(const_iterator before) {
		assert($front);
		assert(before.node());

		return $pop(before.node()->after);
	}

	iterator erase_front() {
		return $erase($front);
	}

	void erase_back() {
		assert($front);

		auto &back = $back();
		delete back;
		back = nullptr;
	}

	iterator erase_after(const_iterator before) {
		assert($front);
		assert(before.node());

		return $erase(before.node()->after);
	}

	T &front() {
		assert($front);

		return $front->value;
	}

	const T &front() const {
		assert($front);

		return $front->value;
	}

	iterator begin() {
		return iterator($front);
	}

	const_iterator begin() const {
		return const_iterator($front);
	}

	RUA_CONSTEXPR_14 iterator end() {
		return iterator();
	}

	constexpr const_iterator end() const {
		return const_iterator();
	}

	void reset() {
		auto n = $front;
		while (n) {
			auto after = n->after;
			delete n;
			n = after;
		}
	}

	node_t *release() {
		auto front = $front;
		$front = nullptr;
		return front;
	}

private:
	node_t *$front;

	T $pop(node_t *&n) {
		assert(n);

		auto after = n->after;
		T r(std::move(n->value));
		delete n;
		n = after;
		return r;
	}

	iterator $erase(node_t *&n) {
		assert(n);

		iterator it(n->after);
		delete n;
		n = it.node();
		return it;
	}

	node_t *&$back() {
		assert($front);

		auto slot = &$front;
		while ((*slot)->after) {
			slot = &(*slot)->after;
		}
		return *slot;
	}
};

} // namespace rua

#endif
