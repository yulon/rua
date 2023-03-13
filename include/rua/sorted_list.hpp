#ifndef _rua_sorted_list_hpp
#define _rua_sorted_list_hpp

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
		return $li.empty();
	}

	size_type size() const {
		return $li.size();
	}

	void emplace(T &&rval) {
		constexpr std::less<T> less{};
		for (auto it = $li.begin(); it != $li.end(); ++it) {
			if (less(rval, *it)) {
				$li.emplace(it, std::move(rval));
				return;
			}
		}
		$li.emplace_back(std::move(rval));
	}

	template <
		typename... Args,
		typename = enable_if_t<!std::is_same<front_t<Args...>, T>::value>>
	void emplace(Args &&...args) {
		emplace(T{std::forward<Args>(args)...});
	}

	iterator erase(const_iterator pos) {
		return $li.erase(std::move(pos));
	}

	iterator erase(const_iterator first, const_iterator last) {
		return $li.erase(std::move(first), std::move(last));
	}

	iterator begin() {
		return $li.begin();
	}

	const_iterator begin() const {
		return $li.begin();
	}

	iterator end() {
		return $li.end();
	}

	const_iterator end() const {
		return $li.end();
	}

	reverse_iterator rbegin() {
		return $li.rbegin();
	}

	const_reverse_iterator rbegin() const {
		return $li.rbegin();
	}

	reverse_iterator rend() {
		return $li.rend();
	}

	const_reverse_iterator rend() const {
		return $li.rend();
	}

	T &front() {
		return $li.front();
	}

	const T &front() const {
		return $li.front();
	}

	T &back() {
		return $li.back();
	}

	const T &back() const {
		return $li.back();
	}

private:
	std::list<T> $li;
};

} // namespace rua

#endif
