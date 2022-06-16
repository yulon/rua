#ifndef _RUA_STRING_SPLIT_HPP
#define _RUA_STRING_SPLIT_HPP

#include "view.hpp"

#include "../util.hpp"

#include <list>
#include <vector>

namespace rua {

template <typename Part>
inline std::vector<Part> _basic_split(
	string_view str,
	string_view sep,
	size_t skip_count = 0,
	int cut_count = nmax<int>()) {
	std::vector<Part> r_vec;
	if (str.empty()) {
		return r_vec;
	}
	std::list<Part> r_li;
	size_t sc = 0;
	auto cc = 0;
	if (cut_count > 0) {
		size_t pos = 0;
		size_t start = 0;
		for (;;) {
			pos = str.find(sep, pos);
			if (pos == string_view::npos) {
				break;
			}
			if (sc == skip_count) {
				r_li.emplace_back(str.substr(start, pos - start));
				pos = pos + sep.length();
				start = pos;
				++cc;
			} else {
				pos = pos + sep.length();
				++sc;
			}
			if (start >= str.length() ||
				(cut_count != nmax<int>() && cc == cut_count)) {
				break;
			}
		}
		if (start <= str.length() && sc == skip_count) {
			r_li.emplace_back(str.substr(start));
		}
	} else {
		auto pos = str.length() - sep.length();
		auto end = str.length();
		for (;;) {
			pos = str.rfind(sep, pos);
			if (pos == string_view::npos) {
				break;
			}
			if (sc == skip_count) {
				auto start = pos + sep.length();
				r_li.emplace_front(str.substr(start, end - start));
				end = pos;
				--cc;
			} else {
				++sc;
			}
			if (!pos || (cut_count != nlowest<int>() && cc == cut_count)) {
				break;
			}
			--pos;
		}
		if (end && sc == skip_count) {
			r_li.emplace_front(str.substr(0, end));
		}
	}
	r_vec.reserve(r_li.size());
	for (auto &part : r_li) {
		r_vec.emplace_back(std::move(part));
	}
	return r_vec;
}

inline std::vector<string_view> split(
	string_view str,
	string_view sep,
	size_t skip_count = 0,
	int cut_count = nmax<int>()) {
	return _basic_split<string_view>(str, sep, skip_count, cut_count);
}

template <
	typename Str,
	typename RcrStr = remove_cvref_t<Str>,
	typename Part = conditional_t<
		std::is_rvalue_reference<Str &&>::value,
		std::string,
		string_view>>
inline enable_if_t<
	std::is_convertible<Str &&, string_view>::value &&
		!std::is_same<RcrStr, string_view>::value,
	std::vector<Part>>
split(
	Str &&str,
	string_view sep,
	size_t skip_count = 0,
	int cut_count = nmax<int>()) {
	return _basic_split<Part>(
		std::forward<Str>(str), sep, skip_count, cut_count);
}

template <typename Str>
inline decltype(split(std::declval<Str &&>(), ""))
split(Str &&str, char sep, size_t skip_count = 0, int cut_count = nmax<int>()) {
	return split(std::forward<Str>(str), {&sep, 1}, skip_count, cut_count);
}

} // namespace rua

#endif
