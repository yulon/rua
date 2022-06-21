#ifndef _RUA_STRING_SLICE_HPP
#define _RUA_STRING_SLICE_HPP

#include "view.hpp"

#include "../util.hpp"

#include <cassert>

namespace rua {

template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> slice(
	basic_string_view<CharT, Traits> str_v,
	ptrdiff_t begin_offset,
	ptrdiff_t end_offset) {
	return str_v.substr(begin_offset, end_offset - begin_offset);
}

template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits>
slice(basic_string_view<CharT, Traits> str_v, ptrdiff_t begin_offset = 0) {
	return str_v.substr(begin_offset);
}

template <typename CharT, typename Traits, typename Allocator>
inline std::basic_string<CharT, Traits, Allocator> slice(
	std::basic_string<CharT, Traits, Allocator> &&str_rr,
	ptrdiff_t begin_offset,
	ptrdiff_t end_offset) {
	if (end_offset == static_cast<ptrdiff_t>(str_rr.size())) {
		assert(!begin_offset);
		return std::move(str_rr);
	}
	return std::move(str_rr).substr(begin_offset, end_offset - begin_offset);
}

template <typename CharT, typename Traits, typename Allocator>
inline std::basic_string<CharT, Traits, Allocator> slice(
	std::basic_string<CharT, Traits, Allocator> &&str_rr,
	ptrdiff_t begin_offset = 0) {
	if (!begin_offset) {
		return std::move(str_rr);
	}
	return std::move(str_rr).substr(begin_offset);
}

template <
	typename StrLike,
	typename RmCvrStrLike = remove_cvref_t<StrLike>,
	typename = enable_if_t<
		std::is_convertible<RmCvrStrLike, std::string>::value &&
		(!std::is_base_of<std::string, RmCvrStrLike>::value ||
		 std::is_lvalue_reference<StrLike &&>::value) &&
		!std::is_base_of<string_view, RmCvrStrLike>::value>>
inline std::string
slice(StrLike &&str_like, ptrdiff_t begin_offset, ptrdiff_t end_offset) {
	return slice(std::string(str_like), begin_offset, end_offset);
}

template <
	typename StrLike,
	typename RmCvrStrLike = remove_cvref_t<StrLike>,
	typename = enable_if_t<
		std::is_convertible<RmCvrStrLike, std::string>::value &&
		(!std::is_base_of<std::string, RmCvrStrLike>::value ||
		 std::is_lvalue_reference<StrLike &&>::value) &&
		!std::is_base_of<string_view, RmCvrStrLike>::value>>
inline std::string slice(StrLike &&str_like, ptrdiff_t begin_offset = 0) {
	return slice(std::string(str_like), begin_offset);
}

template <
	typename StrLike,
	typename RmCvrStrLike = remove_cvref_t<StrLike>,
	typename = enable_if_t<
		std::is_convertible<RmCvrStrLike, std::wstring>::value &&
		(!std::is_base_of<std::wstring, RmCvrStrLike>::value ||
		 std::is_lvalue_reference<StrLike &&>::value) &&
		!std::is_base_of<wstring_view, RmCvrStrLike>::value>>
inline std::wstring
slice(StrLike &&str_like, ptrdiff_t begin_offset, ptrdiff_t end_offset) {
	return slice(std::wstring(str_like), begin_offset, end_offset);
}

template <
	typename StrLike,
	typename RmCvrStrLike = remove_cvref_t<StrLike>,
	typename = enable_if_t<
		std::is_convertible<RmCvrStrLike, std::wstring>::value &&
		(!std::is_base_of<std::wstring, RmCvrStrLike>::value ||
		 std::is_lvalue_reference<StrLike &&>::value) &&
		!std::is_base_of<wstring_view, RmCvrStrLike>::value>>
inline std::wstring slice(StrLike &&str_like, ptrdiff_t begin_offset = 0) {
	return slice(std::wstring(str_like), begin_offset);
}

} // namespace rua

#endif
