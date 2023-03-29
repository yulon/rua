#ifndef _rua_path_hpp
#define _rua_path_hpp

#include "span.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "string/trim.hpp"
#include "string/view.hpp"
#include "util.hpp"

#include <list>
#include <string>

namespace rua {

template <typename Path, char Sep = '/'>
class path_base {
public:
	static constexpr auto sep = Sep;

	template <
		typename... Parts,
		typename = enable_if_t<
			(sizeof...(Parts) > 1) ||
			!std::is_base_of<path_base, decay_t<front_t<Parts...>>>::value>>
	path_base(Parts &&...parts) :
		path_base(std::initializer_list<string_view>(
			{to_temp_string(std::forward<Parts>(parts))...})) {}

	template <
		typename PartList,
		typename = enable_if_t<
			!std::is_base_of<path_base, decay_t<PartList>>::value &&
			!std::is_constructible<string_view, PartList &&>::value>,
		typename = decltype(to_temp_string(
			std::declval<typename span_traits<PartList>::element_type>()))>
	path_base(PartList &&part_list) :
		$s($join(std::forward<PartList>(part_list))) {}

	explicit operator bool() const {
		return $s.length();
	}

	const std::string &str() const & {
		return $s;
	}

	std::string &str() & {
		return $s;
	}

	std::string &&str() && {
		return std::move($s);
	}

	Path add_tail_sep() const {
		if ($s[$s.length() - 1] == Sep) {
			return $s;
		}
		return $s + std::string(1, Sep);
	}

	Path rm_tail_sep() const {
		auto ix = $s.length() - 1;
		if ($s[ix] == Sep) {
			return $s.substr(0, ix);
		}
		return $s;
	}

	std::string back() const {
		auto pos = $s.rfind(Sep);
		return pos != std::string::npos ? $s.substr(pos + 1) : $s;
	}

	Path rm_back() const {
		auto pos = $s.rfind(Sep);
		return pos != std::string::npos ? $s.substr(0, pos) : "";
	}

	std::string front() const {
		auto pos = $s.find(Sep);
		return pos != std::string::npos ? $s.substr(0, pos) : $s;
	}

	Path rm_front() const {
		auto pos = $s.find(Sep);
		return pos != std::string::npos ? $s.substr(pos + 1) : "";
	}

protected:
	path_base() = default;

private:
	std::string $s;

	static bool $is_other_sep(char c) {
		static constexpr string_view other_seps(
			Sep == '/' ? "\\" : (Sep == '\\' ? "/" : "/\\"));

		for (auto other_sep : other_seps) {
			if (c == other_sep) {
				return true;
			}
		}
		return false;
	}

	static bool $is_sep(char c) {
		return c == Sep || $is_other_sep(c);
	}

	static std::string &&$fix_seps(std::string &&s) {
		for (auto &c : s) {
			if ($is_other_sep(c)) {
				c = Sep;
			}
		}
		return std::move(s);
	}

	template <typename PartList>
	static std::string $join(PartList &&part_list) {
		std::list<std::string> parts;
		for (auto &part : std::forward<PartList>(part_list)) {
			parts.emplace_back(trim_right_if(part, $is_sep));
		}
		auto sep = Sep;
		string_view sep_sv(&sep, 1);
		if (parts.front() == sep_sv) {
			auto front = std::move(parts.front());
			parts.pop_front();
			return $fix_seps(front + join(parts, sep_sv, true));
		}
		return $fix_seps(join(parts, sep_sv, true));
	}
};

template <typename Path, char Sep>
inline Path
operator/(const path_base<Path, Sep> &a, const path_base<Path, Sep> &b) {
	return Path(a, b);
}

template <
	typename Path,
	char Sep,
	typename Part,
	typename = enable_if_t<
		!std::is_base_of<path_base<Path, Sep>, decay_t<Part>>::value>>
inline Path operator/(const path_base<Path, Sep> &path, Part &&part) {
	return Path(path, std::forward<Part>(part));
}

template <
	typename Path,
	char Sep,
	typename Part,
	typename = enable_if_t<
		!std::is_base_of<path_base<Path, Sep>, decay_t<Part>>::value>>
inline Path operator/(Part &&part, const path_base<Path, Sep> &path) {
	return Path(std::forward<Part>(part), path);
}

template <typename Path, char Sep, typename Part>
inline Path &operator/=(path_base<Path, Sep> &path, Part &&part) {
	static_cast<Path &>(path) = Path(path, std::forward<Part>(part));
	return static_cast<Path &>(path);
}

template <typename Path, char Sep>
inline const std::string &to_string(const path_base<Path, Sep> &p) {
	return p.str();
}

template <typename Path, char Sep>
inline std::string &to_string(path_base<Path, Sep> &p) {
	return p.str();
}

template <typename Path, char Sep>
inline std::string &&to_string(path_base<Path, Sep> &&p) {
	return std::move(p).str();
}

#define RUA_PATH_CTORS(Path)                                                   \
	Path() = default;                                                          \
                                                                               \
	template <                                                                 \
		typename... Parts,                                                     \
		typename = enable_if_t<                                                \
			(sizeof...(Parts) > 1) ||                                          \
			!std::is_base_of<Path, decay_t<front_t<Parts...>>>::value>>        \
	Path(Parts &&...parts) : path_base(std::forward<Parts>(parts)...) {}

#define RUA_PATH_CTORS_2(Path, Sep)                                            \
	Path() = default;                                                          \
                                                                               \
	template <                                                                 \
		typename... Parts,                                                     \
		typename = enable_if_t<                                                \
			(sizeof...(Parts) > 1) ||                                          \
			!std::is_base_of<Path, decay_t<front_t<Parts...>>>::value>>        \
	Path(Parts &&...parts) :                                                   \
		path_base<Path, Sep>(std::forward<Parts>(parts)...) {}

template <char Sep = '/'>
class basic_path : public path_base<basic_path<Sep>, Sep> {
public:
	RUA_PATH_CTORS_2(basic_path, Sep)
};

using path = basic_path<>;

} // namespace rua

#endif
