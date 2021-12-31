#ifndef _RUA_NET_IP_HPP
#define _RUA_NET_IP_HPP

#include "../bytes.hpp"

#include <cassert>

namespace rua {

class ip {
public:
	constexpr ip() : _v6(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) {}

	constexpr ip(uchar v4_0, uchar v4_1, uchar v4_2, uchar v4_3) :
		_v6(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, v4_0, v4_1, v4_2, v4_3) {}

	constexpr ip(uint32_t v4) :
		ip(v4 >> 24, (v4 >> 16) & 0xFF, (v4 >> 8) & 0xFF, v4 & 0xFF) {}

	constexpr ip(
		uchar v6_0,
		uchar v6_1,
		uchar v6_2,
		uchar v6_3,
		uchar v6_4,
		uchar v6_5,
		uchar v6_6,
		uchar v6_7,
		uchar v6_8,
		uchar v6_9,
		uchar v6_10,
		uchar v6_11,
		uchar v6_12,
		uchar v6_13,
		uchar v6_14,
		uchar v6_15) :
		_v6(v6_0,
			v6_1,
			v6_2,
			v6_3,
			v6_4,
			v6_5,
			v6_6,
			v6_7,
			v6_8,
			v6_9,
			v6_10,
			v6_11,
			v6_12,
			v6_13,
			v6_14,
			v6_15) {}

	constexpr ip(
		uint16_t v6_0,
		uint16_t v6_1,
		uint16_t v6_2,
		uint16_t v6_3,
		uint16_t v6_4,
		uint16_t v6_5,
		uint16_t v6_6,
		uint16_t v6_7) :
		ip(v6_0 >> 8,
		   v6_0 & 0xFF,
		   v6_1 >> 8,
		   v6_1 & 0xFF,
		   v6_2 >> 8,
		   v6_2 & 0xFF,
		   v6_3 >> 8,
		   v6_3 & 0xFF,
		   v6_4 >> 8,
		   v6_4 & 0xFF,
		   v6_5 >> 8,
		   v6_5 & 0xFF,
		   v6_6 >> 8,
		   v6_6 & 0xFF,
		   v6_7 >> 8,
		   v6_7 & 0xFF) {}

	explicit ip(bytes_view b) {
		if (b.size() == 16) {
			_v6.copy(b);
			return;
		}
		if (b.size() == 4) {
			memset(_v6.data(), 0, 10);
			memset(_v6.data() + 10, 0xFF, 2);
			_v6(12).copy(b);
			return;
		}
		assert(!b);
		memset(_v6.data(), 0, 16);
	}

	explicit operator bool() const {
		return !is_unspecified();
	}

	inline bool equal(const ip &other) const {
		return _v6 == other._v6;
	}

	inline bool operator==(const ip &other) const {
		return _v6 == other._v6;
	}

	inline bool is_v4() const;

	bytes_ref v4() {
		assert(is_v4());

		return _v6(12);
	}

	uchar v4(size_t ix) {
		assert(is_v4());

		return _v6(12)[ix];
	}

	bytes_view v4() const {
		assert(is_v4());

		return _v6(12);
	}

	uchar v4(size_t ix) const {
		assert(is_v4());

		return _v6(12)[ix];
	}

	bytes_ref v6() {
		return _v6;
	}

	uchar v6(size_t ix) {
		return _v6[ix];
	}

	bytes_view v6() const {
		return _v6;
	}

	uchar v6(size_t ix) const {
		return _v6[ix];
	}

	inline bool is_unspecified() const;

	inline bool is_loopback() const;

	inline bool is_private() const;

	inline bool is_global_unicast() const;

	inline bool is_public() const;

	inline bool is_multicast() const;

	inline bool is_link_local_unicast() const;

	inline bool is_link_local_multicast() const;

private:
	bytes_block<16> _v6;
};

RUA_INLINE_CONST ip ip4_zero(0, 0, 0, 0);			   // all zeros
RUA_INLINE_CONST ip ip4_loopback(127, 0, 0, 1);		   // limited broadcast
RUA_INLINE_CONST ip ip4_broadcast(255, 255, 255, 255); // limited broadcast
RUA_INLINE_CONST ip ip4_all_systems(224, 0, 0, 1);	   // all systems
RUA_INLINE_CONST ip ip4_all_routers(224, 0, 0, 2);	   // all routers

RUA_INLINE_CONST ip ip6_zero(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
RUA_INLINE_CONST
ip ip6_unspecified(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
RUA_INLINE_CONST
ip ip6_loopback(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
RUA_INLINE_CONST
ip ip6_interface_local_all_nodes(
	0xff, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01);
RUA_INLINE_CONST
ip ip6_link_local_all_nodes(
	0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01);
RUA_INLINE_CONST ip ip6_link_local_all_routers(
	0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02);

inline bool ip::is_v4() const {
	return _v6(0, 12) == ip4_zero._v6(0, 12);
}

inline bool ip::is_unspecified() const {
	return equal(ip4_zero) || equal(ip6_zero);
}

inline bool ip::is_loopback() const {
	return is_v4() ? v4(0) == 127 : equal(ip6_loopback);
}

inline bool ip::is_private() const {
	return is_v4() ? v4(0) == 10 || (v4(0) == 172 && (v4(1) & 0xF0) == 16) ||
						 (v4(0) == 192 && v4(1) == 168)
				   : (_v6[0] & 0xFE) == 0xFC;
}

inline bool ip::is_global_unicast() const {
	return !equal(ip4_broadcast) && !is_unspecified() && !is_loopback() &&
		   !is_multicast() && !is_link_local_unicast();
}

inline bool ip::is_public() const {
	return !is_private() && is_global_unicast();
}

inline bool ip::is_multicast() const {
	return is_v4() ? (v4(0) & 0xF0) == 0xE0 : _v6[0] == 0xFF;
}

inline bool ip::is_link_local_unicast() const {
	return is_v4() ? v4(0) == 169 && v4(1) == 254
				   : _v6[0] == 0xFE && (_v6[1] & 0xC0) == 0x80;
}

inline bool ip::is_link_local_multicast() const {
	return is_v4() ? v4(0) == 224 && v4(1) == 0 && v4(2) == 0
				   : _v6[0] == 0xFF && (_v6[1] & 0x0F) == 0x02;
}

} // namespace rua

#endif
