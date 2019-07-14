#ifndef _RUA_CHRONO_DURATION_HPP
#define _RUA_CHRONO_DURATION_HPP

#include "../macros.hpp"
#include "../str.hpp"
#include "../bin.hpp"

#include <cstdint>

namespace rua {

class duration {
public:
	constexpr duration(int64_t count = 0) : _c(count) {}

	constexpr int64_t count() const {
		return _c;
	}

	constexpr explicit operator bool() const {
		return _c;
	}

	constexpr bool operator!() const {
		return !_c;
	}

	constexpr bool operator==(duration target) const {
		return _c == target.count();
	}

	constexpr bool operator>(duration target) const {
		return _c > target.count();
	}

	constexpr bool operator<(duration target) const {
		return _c < target.count();
	}

	constexpr bool operator>=(duration target) const {
		return _c >= target.count();
	}

	constexpr bool operator<=(duration target) const {
		return _c <= target.count();
	}

	constexpr duration operator+(duration target) const {
		return _c + target.count();
	}

	duration &operator+=(duration target) {
		_c += target.count();
		return *this;
	}

	constexpr duration operator-(duration target) const {
		return _c - target.count();
	}

	constexpr duration operator-() const {
		return -_c;
	}

	duration &operator-=(duration target) {
		_c -= target.count();
		return *this;
	}

	constexpr duration operator*(int64_t target) const {
		return _c * target;
	}

	duration &operator*=(int64_t target) {
		_c *= target;
		return *this;
	}

	constexpr int64_t operator/(duration target) const {
		return _c / target.count();
	}

	constexpr duration operator/(int64_t target) const {
		return _c / target;
	}

	duration &operator/=(int64_t target) {
		_c /= target;
		return *this;
	}

	constexpr duration operator%(duration target) const {
		return _c % target.count();
	}

	duration &operator%=(duration target) {
		_c %= target.count();
		return *this;
	}

public:
	int64_t _c;
};

RUA_FORCE_INLINE constexpr duration operator*(int64_t a, duration b) {
	return a * b.count();
}

namespace chrono_literals {
	constexpr duration operator""_ns(size_t count) {
		return static_cast<int64_t>(count);
	}

	constexpr duration operator""_us(size_t count) {
		return static_cast<int64_t>(count) * 1000;
	}

	constexpr duration operator""_ms(size_t count) {
		return static_cast<int64_t>(count) * 1000000;
	}

	constexpr duration operator""_s(size_t count) {
		return static_cast<int64_t>(count) * 1000000000;
	}

	constexpr duration operator""_min(size_t count) {
		return static_cast<int64_t>(count) * 60000000000;
	}

	constexpr duration operator""_h(size_t count) {
		return static_cast<int64_t>(count) * 3600000000000;
	}
}

using namespace chrono_literals;

static constexpr auto ns = 1_ns;
static constexpr auto us = 1_us;
static constexpr auto ms = 1_ms;
static constexpr auto s = 1_s;
static constexpr auto min = 1_min;
static constexpr auto h = 1_h;

////////////////////////////////////////////////////////////////////////////

// Reference from https://github.com/golang/go/blob/release-branch.go1.12/src/time/time.go#L665

// _duration_to_string_fmt_frac formats the fraction of v/10**prec (e.g., ".12345") into the
// tail of buf, omitting trailing zeros. It omits the decimal
// point too when the fraction is 0. It returns the index where the
// output bytes begin and the value v/10**prec.
inline void _duration_to_string_fmt_frac(bin_ref buf, duration v, int prec, int *nw, duration *nv) {
	// Omit trailing zeros up to and including decimal point.
	auto w = buf.size();
	auto print = false;
	for (auto i = 0; i < prec; i++) {
		auto digit = v.count() % 10;
		print = print || digit != 0;
		if (print) {
			w--;
			buf[w] = static_cast<char>(digit) + '0';
		}
		v = v / 10;
	}
	if (print) {
		w--;
		buf[w] = '.';
	}
	*nw = w;
	*nv = v;
}

// _duration_to_string_fmt_int formats v into the tail of buf.
// It returns the index where the output begins.
inline int _duration_to_string_fmt_int(bin_ref buf, duration v) {
	auto w = buf.size();
	if (v == 0) {
		w--;
		buf[w] = '0';
	} else {
		while (v > 0) {
			w--;
			buf[w] = static_cast<char>(v.count()%10) + '0';
			v /= 10;
		}
	}
	return w;
}

// String returns a string representing the duration in the form "72h3m0.5s".
// Leading zero units are omitted. As a special case, durations less than one
// second format use a smaller unit (milli-, micro-, or nanoseconds) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
inline std::string to_str(duration dur) {
	// Largest time is 2540400h10m10.000000000s
	char b[32];
	int w = sizeof(b);
	bin_ref buf(b, w);

	auto d = dur;
	auto u = d;
	auto neg = d < 0;
	if (neg) {
		u = -u;
	}

	if (u < s) {
		// Special case: if duration is smaller than a second,
		// use smaller units, like 1.2ms
		int prec;
		w--;
		buf[w] = 's';
		w--;
		if (u == 0) {
			return "0s";
		} else if (u < us) {
			// print nanoseconds
			prec = 0;
			buf[w] = 'n';
		} else if (u < ms) {
			// print microseconds
			prec = 3;
			// U+00B5 'µ' micro sign == 0xC2 0xB5
			w--; // Need room for two bytes.
			buf(w).copy("µ");
		} else {
			// print milliseconds
			prec = 6;
			buf[w] = 'm';
		}
		_duration_to_string_fmt_frac(buf(0, w), u, prec, &w, &u);
		w = _duration_to_string_fmt_int(buf(0, w), u);
	} else {
		w--;
		buf[w] = 's';

		_duration_to_string_fmt_frac(buf(0, w), u, 9, &w, &u);

		// u is now integer seconds
		w = _duration_to_string_fmt_int(buf(0, w), u % 60);
		u = u/60;

		// u is now integer minutes
		if (u > 0) {
			w--;
			buf[w] = 'm';
			w = _duration_to_string_fmt_int(buf(0, w), u % 60);
			u /= 60;

			// u is now integer hours
			// Stop at hours because days can be different lengths.
			if (u > 0) {
				w--;
				buf[w] = 'h';
				w = _duration_to_string_fmt_int(buf(0, w), u);
			}
		}
	}

	if (neg) {
		w--;
		buf[w] = '-';
	}

	buf = buf(w);
	return std::string(buf.base(), buf.size());
}

}

#endif
