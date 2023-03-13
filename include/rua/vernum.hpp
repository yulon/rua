#ifndef _rua_vernum_hpp
#define _rua_vernum_hpp

#include "string/conv.hpp"
#include "string/join.hpp"
#include "util.hpp"

namespace rua {

class vernum {
public:
	constexpr vernum(
		uint16_t major = 0,
		uint16_t minor = 0,
		uint16_t build = 0,
		uint16_t revision = 0) :
		_whole(
			(static_cast<uint64_t>(major) << 48) |
			(static_cast<uint64_t>(minor) << 32) |
			(static_cast<uint64_t>(build) << 16) |
			static_cast<uint64_t>(revision)) {}

	RUA_CONSTEXPR_14 uint64_t &whole() {
		return _whole;
	}

	constexpr uint64_t whole() const {
		return _whole;
	}

	constexpr uint16_t operator[](size_t ix) const {
		return static_cast<uint16_t>(_whole >> (48 - ix * 16) & 0xFFFF);
	}

	constexpr bool operator==(const vernum &target) const {
		return _whole == target._whole;
	}

	constexpr bool operator!=(const vernum &target) const {
		return _whole != target._whole;
	}

	constexpr bool operator>(const vernum &target) const {
		return _whole > target._whole;
	}

	constexpr bool operator>=(const vernum &target) const {
		return _whole >= target._whole;
	}

	constexpr bool operator<(const vernum &target) const {
		return _whole < target._whole;
	}

	constexpr bool operator<=(const vernum &target) const {
		return _whole <= target._whole;
	}

private:
	uint64_t _whole;
};

inline std::string to_string(const vernum &vn) {
	return join(
		{to_string(vn.whole() >> 48),
		 to_string(vn.whole() >> 32 & 0xFFFF),
		 to_string(vn.whole() >> 16 & 0xFFFF),
		 to_string(vn.whole() & 0xFFFF)},
		'.');
}

} // namespace rua

#endif
