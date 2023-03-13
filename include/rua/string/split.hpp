#ifndef _rua_string_split_hpp
#define _rua_string_split_hpp

#include "slice.hpp"
#include "view.hpp"

#include "../util.hpp"

#include <list>
#include <vector>

namespace rua {

template <
	typename StrLike,
	typename SepStrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>())),
	typename SepStrView = decltype(view_string(std::declval<SepStrLike &&>())),
	typename Part = decltype(slice(std::declval<StrLike &&>()))>
inline std::vector<Part> split(
	StrLike &&str_like,
	SepStrLike &&sep_str_like,
	size_t skip_count = 0,
	int cut_count = nmax<int>()) {
	std::vector<Part> r_vec;
	StrView str_v(std::forward<StrLike>(str_like));
	if (str_v.empty()) {
		return r_vec;
	}
	SepStrView sep_str_v(std::forward<SepStrLike>(sep_str_like));
	if (sep_str_v.empty()) {
		return r_vec;
	}
	std::list<Part> r_li;
	size_t sc = 0;
	auto cc = 0;
	if (cut_count > 0) {
		size_t pos = 0;
		size_t start = 0;
		for (;;) {
			pos = str_v.find(sep_str_v, pos);
			if (pos == StrView::npos) {
				break;
			}
			if (sc == skip_count) {
				r_li.emplace_back(str_v.substr(start, pos - start));
				pos = pos + sep_str_v.length();
				start = pos;
				++cc;
			} else {
				pos = pos + sep_str_v.length();
				++sc;
			}
			if (start >= str_v.length() ||
				(cut_count != nmax<int>() && cc == cut_count)) {
				break;
			}
		}
		if (start <= str_v.length() && sc == skip_count) {
			r_li.emplace_back(str_v.substr(start));
		}
	} else {
		auto pos = str_v.length() - sep_str_v.length();
		auto end = str_v.length();
		for (;;) {
			pos = str_v.rfind(sep_str_v, pos);
			if (pos == StrView::npos) {
				break;
			}
			if (sc == skip_count) {
				auto start = pos + sep_str_v.length();
				r_li.emplace_front(str_v.substr(start, end - start));
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
			r_li.emplace_front(str_v.substr(0, end));
		}
	}
	r_vec.reserve(r_li.size());
	for (auto &part : r_li) {
		r_vec.emplace_back(std::move(part));
	}
	return r_vec;
}

template <
	typename StrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>())),
	typename Char = typename StrView::value_type,
	typename Part = decltype(slice(std::declval<StrLike &&>()))>
inline std::vector<Part> split(
	StrLike &&str_like,
	type_identity_t<Char> sep_char,
	size_t skip_count = 0,
	int cut_count = nmax<int>()) {
	return split<StrLike, StrView, StrView, StrView, Part>(
		std::forward<StrLike>(str_like),
		StrView(&sep_char, 1),
		skip_count,
		cut_count);
}

} // namespace rua

#endif
