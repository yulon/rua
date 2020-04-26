#ifndef _RUA_PATH_HPP
#define _RUA_PATH_HPP

#include "macros.hpp"
#include "span.hpp"
#include "string/join.hpp"
#include "string/view.hpp"
#include "types/traits.hpp"

#include <string>
#include <vector>

namespace rua {

template <char Sep>
class basic_path {
public:
	static constexpr auto sep = Sep;

	basic_path() = default;

	template <
		typename... Parts,
		typename = enable_if_t<
			(sizeof...(Parts) > 1) ||
			!std::is_base_of<basic_path, decay_t<argments_front_t<Parts...>>>::
				value>>
	basic_path(Parts &&... parts) :
		_s(str_join(
			_fix_parts({to_string(std::forward<Parts>(parts))...}),
			Sep,
			RUA_DI(str_join_options, $.is_ignore_empty = true))) {}

	template <
		typename PartList,
		typename = enable_if_t<
			!std::is_base_of<basic_path, decay_t<PartList>>::value &&
			!std::is_convertible<PartList &&, string_view>::value>,
		typename = decltype(to_string(
			std::declval<typename span_traits<PartList>::element_type>()))>
	basic_path(PartList &&part_list) :
		_s(str_join(
			_fix_part_list(std::forward<PartList>(part_list)),
			Sep,
			RUA_DI(str_join_options, $.is_ignore_empty = true))) {}

	explicit operator bool() const {
		return _s.length();
	}

	const std::string &string() const & {
		return _s;
	}

	const std::string &string() & {
		return _s;
	}

	std::string &&string() && {
		return std::move(_s);
	}

private:
	std::string _s;

	static void _fix_seps(std::string &part) {
		static constexpr string_view other_seps(
			Sep == '/' ? "\\" : (Sep == '\\' ? "/" : "/\\"));

		for (auto &c : part) {
			for (auto &other_sep : other_seps) {
				if (c == other_sep) {
					c = Sep;
					break;
				}
			}
		}
	}

	static std::vector<std::string> &&
	_fix_parts(std::vector<std::string> &&parts) {
		for (auto &part : parts) {
			_fix_seps(part);
		}
		return std::move(parts);
	}

	template <typename PartList>
	static std::vector<std::string> _fix_part_list(PartList &&part_list) {
		std::vector<std::string> parts;
		parts.reserve(size(std::forward<PartList>(part_list)));
		for (auto &part : std::forward<PartList>(part_list)) {
			auto part_str = to_string(std::move(part));
			_fix_seps(part_str);
			parts.emplace_back(std::move(part_str));
		}
		return parts;
	}
};

template <char Sep>
inline std::string to_string(basic_path<Sep> p) {
	return std::move(p).string();
}

template <char Sep>
inline const std::string &to_tmp_string(const basic_path<Sep> &p) {
	return p.string();
}

using path = basic_path<'/'>;

} // namespace rua

#endif
