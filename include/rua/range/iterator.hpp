#ifndef _RUA_RANGE_ITERATOR_HPP
#define _RUA_RANGE_ITERATOR_HPP

#include "../types/traits.hpp"

#include <iterator>

namespace rua {

struct wandering_iterator {
protected:
	wandering_iterator() = default;
};

template <typename T, typename It = remove_cvref_t<T>>
inline constexpr rua::enable_if_t<
	std::is_base_of<wandering_iterator, It>::value &&
		std::is_copy_constructible<It>::value,
	T>
begin(T &it) {
	return std::forward<T>(it);
}

template <typename T, typename It = remove_cvref_t<T>>
inline constexpr rua::enable_if_t<
	std::is_base_of<wandering_iterator, It>::value &&
		!std::is_copy_constructible<It>::value,
	T>
begin(T &it) {
	return std::move(it);
}

template <typename T, typename It = remove_cvref_t<T>>
inline constexpr rua::
	enable_if_t<std::is_base_of<wandering_iterator, It>::value, T>
	begin(T &&it) {
	return std::move(it);
}

template <typename T, typename Iter = remove_cvref_t<T>>
inline constexpr rua::
	enable_if_t<std::is_base_of<wandering_iterator, Iter>::value, Iter>
	end(T &&) {
	return {};
}

////////////////////////////////////////////////////////////////////////////

template <typename Iter>
class reverse_iterator : public Iter {
public:
	constexpr reverse_iterator() = default;
	constexpr explicit reverse_iterator(Iter &&it) : Iter(std::move(it)) {}
	constexpr explicit reverse_iterator(const Iter &it) : Iter(it) {}

	reverse_iterator &operator++() {
		--*static_cast<Iter *>(this);
		return *this;
	}

	reverse_iterator operator++(int) {
		return reverse_iterator(*static_cast<Iter *>(this)--);
	}

	reverse_iterator operator+(ptrdiff_t offset) {
		return reverse_iterator(*static_cast<Iter *>(this) - offset);
	}

	reverse_iterator &operator+=(ptrdiff_t offset) {
		*static_cast<Iter *>(this) -= offset;
		return *this;
	}

	reverse_iterator &operator--() {
		++*static_cast<Iter *>(this);
		return *this;
	}

	reverse_iterator operator--(int) {
		return reverse_iterator(*static_cast<Iter *>(this)++);
	}

	reverse_iterator operator-(ptrdiff_t offset) {
		return reverse_iterator(*static_cast<Iter *>(this) + offset);
	}

	reverse_iterator &operator-=(ptrdiff_t offset) {
		*static_cast<Iter *>(this) += offset;
		return *this;
	}
};

template <typename Value>
class reverse_iterator<Value *> {
public:
	constexpr reverse_iterator() = default;
	constexpr explicit reverse_iterator(Value *ptr) : _ptr(ptr) {}

	Value &operator*() const {
		return *_ptr;
	}

	Value *operator->() const {
		return _ptr;
	}

	reverse_iterator &operator++() {
		--_ptr;
		return *this;
	}

	reverse_iterator operator++(int) {
		return reverse_iterator(_ptr--);
	}

	reverse_iterator operator+(ptrdiff_t offset) {
		return reverse_iterator(_ptr - offset);
	}

	reverse_iterator &operator+=(ptrdiff_t offset) {
		_ptr -= offset;
		return *this;
	}

	reverse_iterator &operator--() {
		++_ptr;
		return *this;
	}

	reverse_iterator operator--(int) {
		return reverse_iterator(_ptr++);
	}

	reverse_iterator operator-(ptrdiff_t offset) {
		return reverse_iterator(_ptr + offset);
	}

	reverse_iterator &operator-=(ptrdiff_t offset) {
		_ptr += offset;
		return *this;
	}

private:
	Value *_ptr;
};

} // namespace rua

#endif
