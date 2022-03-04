#ifndef _RUA_SORTED_LIST_HPP
#define _RUA_SORTED_LIST_HPP

#include "util.hpp"

#include <functional>
#include <list>

namespace rua {

template <typename T>
class sorted_list {
public:
	using iterator = typename std::list<T>::iterator;
	using const_iterator = typename std::list<T>::const_iterator;
	using reverse_iterator = typename std::list<T>::reverse_iterator;
	using const_reverse_iterator =
		typename std::list<T>::const_reverse_iterator;
	using size_type = typename std::list<T>::size_type;

	sorted_list() = default;

	bool empty() const {
		return _li.empty();
	}

	size_type size() const {
		return _li.size();
	}

	void emplace(T &&rval) {
		constexpr std::less<T> less{};
		for (auto it = _li.begin(); it != _li.end(); ++it) {
			if (less(rval, *it)) {
				_li.emplace(it, std::move(rval));
				return;
			}
		}
		_li.emplace_back(std::move(rval));
	}

	template <
		typename... Args,
		typename = enable_if_t<!std::is_same<front_t<Args...>, T>::value>>
	void emplace(Args &&...args) {
		emplace(T{std::forward<Args>(args)...});
	}

	iterator erase(const_iterator pos) {
		return _li.erase(std::move(pos));
	}

	iterator erase(const_iterator first, const_iterator last) {
		return _li.erase(std::move(first), std::move(last));
	}

	iterator begin() {
		return _li.begin();
	}

	const_iterator begin() const {
		return _li.begin();
	}

	iterator end() {
		return _li.end();
	}

	const_iterator end() const {
		return _li.end();
	}

	reverse_iterator rbegin() {
		return _li.rbegin();
	}

	const_reverse_iterator rbegin() const {
		return _li.rbegin();
	}

	reverse_iterator rend() {
		return _li.rend();
	}

	const_reverse_iterator rend() const {
		return _li.rend();
	}

	T &front() {
		return _li.front();
	}

	const T &front() const {
		return _li.front();
	}

	T &back() {
		return _li.back();
	}

	const T &back() const {
		return _li.back();
	}

private:
	std::list<T> _li;
};

} // namespace rua

#endif
