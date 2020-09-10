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

template <typename Path, char Sep = '/'>
class path_base {
public:
	static constexpr auto sep = Sep;

	path_base() = default;

	template <
		typename... Parts,
		typename = enable_if_t<
			(sizeof...(Parts) > 1) ||
			!std::is_base_of<path_base, decay_t<argments_front_t<Parts...>>>::
				value>>
	path_base(Parts &&... parts) :
		_s(str_join(
			_fix_parts({to_string(std::forward<Parts>(parts))...}),
			Sep,
			RUA_DI(str_join_options, $.is_ignore_empty = true))) {}

	template <
		typename PartList,
		typename = enable_if_t<
			!std::is_base_of<path_base, decay_t<PartList>>::value &&
			!std::is_convertible<PartList &&, string_view>::value>,
		typename = decltype(to_string(
			std::declval<typename span_traits<PartList>::element_type>()))>
	path_base(PartList &&part_list) :
		_s(str_join(
			_fix_part_list(std::forward<PartList>(part_list)),
			Sep,
			RUA_DI(str_join_options, $.is_ignore_empty = true))) {}

	explicit operator bool() const {
		return _s.length();
	}

	const std::string &str() const & {
		return _s;
	}

	std::string &str() & {
		return _s;
	}

	std::string &&str() && {
		return std::move(_s);
	}

	Path add_tail_sep() const {
		if (_s[_s.length() - 1] == Sep) {
			return _s;
		}
		return _s + std::string(1, Sep);
	}

	Path rm_tail_sep() const {
		auto ix = _s.length() - 1;
		if (_s[ix] == Sep) {
			return _s.substr(0, ix);
		}
		return _s;
	}

	std::string back() const {
		auto pos = _s.find_last_of(Sep);
		return pos != std::string::npos ? _s.substr(pos, _s.length() - i) : _s;
										: _s;
	}

	Path rm_back() const {
		auto pos = _s.find_last_of(Sep);
		return pos != std::string::npos ? _s.substr(0, pos) : "";
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

template <typename Path, char Sep>
inline std::string to_string(path_base<Path, Sep> p) {
	return std::move(p).str();
}

template <typename Path, char Sep>
inline const std::string &to_tmp_string(const path_base<Path, Sep> &p) {
	return p.str();
}

#define RUA_PATH_CTOR(Path)                                                    \
	template <                                                                 \
		typename... Parts,                                                     \
		typename = enable_if_t<                                                \
			!(sizeof...(Parts)) || (sizeof...(Parts) > 1) ||                   \
			!std::is_base_of<Path, decay_t<argments_front_t<Parts...>>>::      \
				value>>                                                        \
	Path(Parts &&... parts) : path_base(std::forward<Parts>(parts)...) {}

} // namespace rua

#endif
