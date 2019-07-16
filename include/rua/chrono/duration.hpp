#ifndef _RUA_CHRONO_DURATION_HPP
#define _RUA_CHRONO_DURATION_HPP

#include "../bin.hpp"
#include "../limits.hpp"
#include "../macros.hpp"
#include "../str.hpp"

#include <cstdint>
#include <type_traits>

namespace rua {

class duration_base {};

template <
	int64_t Multiple,
	typename = typename std::enable_if<(Multiple > 0)>::type>
class duration : public duration_base {
public:
	static constexpr int64_t multiple = Multiple;

	constexpr duration(int64_t count = 0) : _c(count) {}

	template <
		typename Duration,
		typename = typename std::enable_if<
			std::is_base_of<duration_base, Duration>::value &&
			!std::is_same<Duration, duration>::value>::type>
	constexpr duration(Duration dur) :
		_c(Duration::multiple / multiple * dur.count()) {}

	constexpr int64_t count() const {
		return _c;
	}

	constexpr explicit operator bool() const {
		return _c;
	}

	constexpr bool operator!() const {
		return !_c;
	}

	constexpr duration operator-() const {
		return -_c;
	}

	duration &operator+=(duration target) {
		_c += target.count();
		return *this;
	}

	duration &operator-=(duration target) {
		_c -= target.count();
		return *this;
	}

	duration &operator*=(duration target) {
		_c *= target.count();
		return *this;
	}

	duration &operator/=(duration target) {
		_c /= target.count();
		return *this;
	}

	duration &operator%=(duration target) {
		_c %= target.count();
		return *this;
	}

public:
	int64_t _c;
};

class _fake_duration {
public:
	static constexpr int64_t multiple = -1;
};

#define RUA_DURATION_CONCEPT(Duration)                                         \
	typename Duration,                                                         \
		typename = typename std::enable_if<                                    \
			std::is_base_of<duration_base, Duration>::value>::type

#define RUA_DURATION_PAIR_CONCEPT                                              \
	typename A, typename B,                                                    \
                                                                               \
		typename = typename std::enable_if <                                   \
					   std::is_base_of<duration_base, A>::value ||             \
				   std::is_base_of<duration_base, B>::value > ::type,          \
                                                                               \
		typename DurationA = typename std::conditional<                        \
			std::is_base_of<duration_base, A>::value,                          \
			A,                                                                 \
			_fake_duration>::type,                                             \
                                                                               \
		typename DurationB = typename std::conditional<                        \
			std::is_base_of<duration_base, B>::value,                          \
			B,                                                                 \
			_fake_duration>::type,                                             \
                                                                               \
		typename DurationC =                                                   \
			typename std::conditional < (DurationA::multiple > 0) &&           \
			(((DurationB::multiple > 0) &&                                     \
			  DurationA::multiple <= DurationB::multiple) ||                   \
			 (DurationB::multiple <= 0)),                                      \
		DurationA, DurationB > ::type

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr bool operator==(A a, B b) {
	return DurationC(a).count() == DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr bool operator>(A a, B b) {
	return DurationC(a).count() > DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr bool operator<(A a, B b) {
	return DurationC(a).count() < DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr bool operator>=(A a, B b) {
	return DurationC(a).count() >= DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr bool operator<=(A a, B b) {
	return DurationC(a).count() <= DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr DurationC operator+(A a, B b) {
	return DurationC(a).count() + DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr DurationC operator-(A a, B b) {
	return DurationC(a).count() - DurationC(b).count();
}

template <RUA_DURATION_CONCEPT(Duration)>
RUA_FORCE_INLINE constexpr Duration operator*(Duration a, int64_t b) {
	return a.count() * b;
}

template <RUA_DURATION_CONCEPT(Duration)>
RUA_FORCE_INLINE constexpr Duration operator*(int64_t a, Duration b) {
	return a * b.count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr int64_t operator/(A a, B b) {
	return DurationC(a).count().count() / DurationC(b).count();
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr DurationC operator/(A a, int64_t b) {
	return DurationC(a).count() / b;
}

template <RUA_DURATION_PAIR_CONCEPT>
RUA_FORCE_INLINE constexpr DurationC operator%(A a, B b) {
	return DurationC(a).count() % DurationC(b).count();
}

using ns = duration<1>;
using us = duration<1000 * ns::multiple>;
using ms = duration<1000 * us::multiple>;
using s = duration<1000 * ms::multiple>;
using min = duration<60 * s::multiple>;
using h = duration<60 * min::multiple>;
using day = duration<24 * h::multiple>;
using week = duration<7 * day::multiple>;

namespace duration_literals {

RUA_FORCE_INLINE constexpr ns operator""_ns(size_t count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr us operator""_us(size_t count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr ms operator""_ms(size_t count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr s operator""_s(size_t count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr min operator""_min(size_t count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr h operator""_h(size_t count) {
	return static_cast<int64_t>(count);
}

} // namespace duration_literals

using namespace duration_literals;

////////////////////////////////////////////////////////////////////////////

// Reference from
// https://github.com/golang/go/blob/release-branch.go1.12/src/time/time.go#L665

// _duration_to_string_fmt_frac formats the fraction of v/10**prec (e.g.,
// ".12345") into the tail of buf, omitting trailing zeros. It omits the decimal
// point too when the fraction is 0. It returns the index where the
// output bytes begin and the value v/10**prec.
inline void
_duration_to_string_fmt_frac(bin_ref buf, ns v, int prec, int *nw, ns *nv) {
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
		v /= 10;
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
inline int _duration_to_string_fmt_int(bin_ref buf, ns v) {
	auto w = buf.size();
	if (v == 0) {
		w--;
		buf[w] = '0';
	} else {
		while (v > 0) {
			w--;
			buf[w] = static_cast<char>(v.count() % 10) + '0';
			v /= 10;
		}
	}
	return w;
}

// String returns a string representing the duration in the form "72h3m0.5s".
// Leading zero units are omitted. As a special case, durations less than one
// second format use a smaller unit (milli-, micro-, or nanoseconds) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
inline std::string to_str(ns dur) {
	// Largest time is 2540400h10m10.000000000s
	char b[32];
	int w = sizeof(b);
	bin_ref buf(b, w);

	auto u = dur;
	auto neg = dur < 0;
	if (neg) {
		u = -u;
	}

	if (u < 1_s) {
		// Special case: if duration is smaller than a second,
		// use smaller units, like 1.2ms
		int prec;
		w--;
		buf[w] = 's';
		w--;
		if (u == 0) {
			return "0s";
		} else if (u < 1_us) {
			// print nanoseconds
			prec = 0;
			buf[w] = 'n';
		} else if (u < 1_ms) {
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
		u /= 60;

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

} // namespace rua

#endif
