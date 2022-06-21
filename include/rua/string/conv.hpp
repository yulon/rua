#ifndef _RUA_STRING_CONV_HPP
#define _RUA_STRING_CONV_HPP

#include "char_codec/base.hpp"
#include "join.hpp"
#include "view.hpp"

#include "../range/traits.hpp"
#include "../span.hpp"
#include "../util.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace rua {

template <typename Char>
inline constexpr enable_if_t<
	std::is_same<remove_cv_t<Char>, char>::value,
	string_view>
as_string(Char *c_str) {
	return c_str;
}

inline constexpr string_view as_string(const char *c_str, size_t size) {
	return {c_str, size};
}

template <typename StringLike, typename SpanTraits = span_traits<StringLike &&>>
inline constexpr enable_if_t<
	std::is_same<typename SpanTraits::value_type, char>::value &&
		!std::is_array<remove_reference_t<StringLike>>::value,
	string_view>
as_string(StringLike &&str) {
	return {
		data(std::forward<StringLike>(str)),
		size(std::forward<StringLike>(str))};
}

template <typename NullPtr>
inline constexpr enable_if_t<is_null_pointer<NullPtr>::value, string_view>
as_string(NullPtr) {
	return "null";
}

template <typename Bool>
inline constexpr enable_if_t<std::is_same<Bool, bool>::value, string_view>
as_string(Bool val) {
	return val ? "true" : "false";
}

template <typename T, typename = void>
struct is_convertible_as_string {
	static constexpr auto value = false;
};

template <typename T>
struct is_convertible_as_string<
	T,
	void_t<decltype(as_string(std::declval<T>()))>> {
	static constexpr auto value = true;
};

////////////////////////////////////////////////////////////////////////////

struct disable_as_to_string {
protected:
	disable_as_to_string() = default;
};

template <typename T, typename DecayT = decay_t<T>>
inline enable_if_t<
	is_convertible_as_string<T &&>::value &&
		!std::is_base_of<std::string, DecayT>::value &&
		!std::is_base_of<disable_as_to_string, DecayT>::value,
	std::string>
to_string(T &&val) {
	auto sv = as_string(std::forward<T>(val));
	return {sv.data(), sv.size()};
}

inline std::string to_string(std::string s) {
	return std::string(std::move(s));
}

template <typename WChar>
inline enable_if_t<
	std::is_same<remove_cv_t<WChar>, wchar_t>::value,
	std::string>
to_string(WChar *c_wstr) {
	return w2u(c_wstr);
}

inline std::string to_string(const wchar_t *c_wstr, size_t size) {
	return w2u({c_wstr, size});
}

template <
	typename WStringLike,
	typename SpanTraits = span_traits<WStringLike &&>>
inline constexpr enable_if_t<
	std::is_same<typename SpanTraits::value_type, wchar_t>::value &&
		!std::is_array<remove_reference_t<WStringLike>>::value,
	std::string>
to_string(WStringLike &&wstr) {
	return w2u(
		{data(std::forward<WStringLike>(wstr)),
		 size(std::forward<WStringLike>(wstr))});
}

template <typename Num>
inline enable_if_t<
	std::is_integral<decay_t<Num>>::value ||
		std::is_floating_point<decay_t<Num>>::value,
	std::string>
to_string(Num &&num) {
	return std::to_string(num);
}

template <typename T>
inline enable_if_t<!std::is_same<decay_t<T>, unsigned char>::value, std::string>
to_hex(T val, size_t width = sizeof(T) * 2) {
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setw(width)
	   << std::setfill('0') << val;
	return ss.str();
}

inline std::string
to_hex(unsigned char val, size_t width = sizeof(unsigned char) * 2) {
	return to_hex(static_cast<uintptr_t>(val), width);
}

template <
	typename T,
	typename Ptr = remove_cvref_t<T>,
	typename Value = remove_cv_t<remove_pointer_t<Ptr>>>
inline enable_if_t<
	std::is_pointer<Ptr>::value && !std::is_same<Value, char>::value &&
		!std::is_same<Value, wchar_t>::value,
	std::string>
to_string(T &&ptr) {
	return ptr ? to_hex(reinterpret_cast<uintptr_t>(ptr)) : to_string(nullptr);
}

template <typename Range, typename RangeTraits = range_traits<Range &&>>
inline enable_if_t<
	!std::is_same<typename RangeTraits::value_type, char>::value &&
		!std::is_same<typename RangeTraits::value_type, wchar_t>::value,
	std::string>
to_string(Range &&);

template <typename T, typename = void>
struct is_convertible_to_string {
	static constexpr auto value = false;
};

template <typename T>
struct is_convertible_to_string<
	T,
	void_t<decltype(to_string(std::declval<T>()))>> {
	static constexpr auto value = true;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr inline enable_if_t<
	is_convertible_as_string<T &&>::value &&
		!std::is_base_of<disable_as_to_string, decay_t<T>>::value,
	string_view>
to_temp_string(T &&val) {
	return as_string(std::forward<T>(val));
}

template <typename T>
inline enable_if_t<
	is_convertible_to_string<T &&>::value &&
		(!is_convertible_as_string<T &&>::value ||
		 std::is_base_of<disable_as_to_string, decay_t<T>>::value),
	std::string>
to_temp_string(T &&val) {
	return to_string(std::forward<T>(val));
}

////////////////////////////////////////////////////////////////////////////

template <typename Range, typename RangeTraits>
inline enable_if_t<
	!std::is_same<typename RangeTraits::value_type, char>::value &&
		!std::is_same<typename RangeTraits::value_type, wchar_t>::value,
	std::string>
to_string(Range &&range) {
	std::string buf;
	buf += '{';
	bool is_first = true;
	for (auto &val : std::forward<Range>(range)) {
		if (is_first) {
			is_first = false;
		} else {
			buf += ", ";
		}
		buf += to_temp_string(val);
	}
	buf += '}';
	return buf;
}

} // namespace rua

#endif
