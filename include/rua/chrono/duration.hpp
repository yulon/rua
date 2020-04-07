#ifndef _RUA_CHRONO_DURATION_HPP
#define _RUA_CHRONO_DURATION_HPP

#include "../bytes.hpp"
#include "../macros.hpp"
#include "../string/str_len.hpp"
#include "../string/to_string.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <cstring>

namespace rua {

template <int64_t Multiple>
class duration {

	RUA_SASSERT(Multiple > 0);

public:
	static constexpr int64_t multiple = Multiple;

	static constexpr duration
	from_s_and_extra_ns_counts(int64_t s_count, int32_t extra_ns_count) {
		return duration(s_count, extra_ns_count);
	}

#ifdef RUA_CONSTEXPR_14_SUPPORTED

	static constexpr duration
	from_s_and_ns_counts(int64_t s_count, int64_t ns_count) {
		auto carrys = ns_count / 1000000000;
		if (carrys != 0) {
			s_count += carrys;
			ns_count %= 1000000000;
		}
		if (s_count < 0 && ns_count > 0) {
			++s_count;
			ns_count = -(1000000000 - ns_count);
		} else if (s_count > 0 && ns_count < 0) {
			--s_count;
			ns_count = 1000000000 + ns_count;
		}
		return duration(s_count, static_cast<int32_t>(ns_count));
	}

#else

private:
	static RUA_FORCE_INLINE constexpr duration
	_from_s_and_ns_counts__pan(int64_t s_count, int64_t ns_count) {
		return (s_count < 0 && ns_count > 0)
				   ? duration(
						 s_count + 1,
						 static_cast<int32_t>(-(1000000000 - ns_count)))
				   : ((s_count > 0 && ns_count < 0)
						  ? duration(
								s_count - 1,
								static_cast<int32_t>(1000000000 + ns_count))
						  : duration(s_count, static_cast<int32_t>(ns_count)));
	}

	static RUA_FORCE_INLINE constexpr duration _from_s_and_ns_counts__overflow(
		int64_t s_count, int64_t ns_count, int64_t carrys, int64_t remainders) {
		return (carrys != 0)
				   ? _from_s_and_ns_counts__pan(s_count + carrys, remainders)
				   : _from_s_and_ns_counts__pan(s_count, ns_count);
	}

public:
	static constexpr duration
	from_s_and_ns_counts(int64_t s_count, int64_t ns_count) {
		return _from_s_and_ns_counts__overflow(
			s_count, ns_count, ns_count / 1000000000, ns_count % 1000000000);
	}

#endif

	////////////////////////////////////////////////////////////////////////

	constexpr duration() : _s(0), _ns(0) {}

	constexpr duration(int64_t count) : duration(_in_attr{}, count) {}

	template <
		int64_t OtherMultiple,
		typename = enable_if_t<OtherMultiple != Multiple>>
	constexpr duration(duration<OtherMultiple> other) :
		duration(other.s_count(), other.extra_ns_count()) {}

	constexpr int64_t count() const {
		return _c(_out_attr{}, _s, _ns);
	}

	constexpr int64_t s_count() const {
		return _s;
	}

	constexpr int32_t extra_ns_count() const {
		return _ns;
	}

	constexpr int64_t ns_count() const {
		return _s * 1000000000 + _ns;
	}

	constexpr explicit operator bool() const {
		return _s || _ns;
	}

	constexpr bool operator==(duration target) const {
		return _s == target._s && _ns == target._ns;
	}

	constexpr bool operator!=(duration target) const {
		return _s != target._s || _ns != target._ns;
	}

	constexpr bool operator>(duration target) const {
		return _s > target._s || (_s == target._s && _ns > target._ns);
	}

	constexpr bool operator<(duration target) const {
		return _s < target._s || (_s == target._s && _ns < target._ns);
	}

	constexpr bool operator>=(duration target) const {
		return _s >= target._s || (_s == target._s && _ns >= target._ns);
	}

	constexpr bool operator<=(duration target) const {
		return _s <= target._s || (_s == target._s && _ns <= target._ns);
	}

	constexpr duration operator+(duration target) const {
		return from_s_and_ns_counts(
			_s + target._s,
			static_cast<int64_t>(_ns) + static_cast<int64_t>(target._ns));
	}

	duration &operator+=(duration target) {
		return *this = *this + target;
	}

	constexpr duration operator-(duration target) const {
		return from_s_and_ns_counts(
			_s - target._s,
			static_cast<int64_t>(_ns) - static_cast<int64_t>(target._ns));
	}

	constexpr duration operator-() const {
		return duration(-_s, -_ns);
	}

	duration &operator-=(duration target) {
		return *this = *this - target;
	}

	constexpr duration operator*(int64_t target) const {
		return from_s_and_ns_counts(
			_s * target, static_cast<int64_t>(_ns) * target);
	}

	duration &operator*=(int64_t target) {
		return *this = *this * target;
	}

	int64_t operator/(duration target) const {
		return *this > target ? _div_ns_count(target.ns_count()) : 0;
	}

	constexpr duration operator/(int64_t target) const {
		return from_s_and_ns_counts(
			_s / target,
			(static_cast<int64_t>(_ns) + _s % target * 1000000000) / target);
	}

	duration &operator/=(int64_t target) {
		return *this = *this / target;
	}

	constexpr duration operator%(duration target) const {
		return from_s_and_ns_counts(
			0,
			target._ns ? (static_cast<int64_t>(_ns) +
						  ((target._s || !target._ns) ? _s % target._s : 0) *
							  1000000000) %
							 target._ns
					   : (static_cast<int64_t>(_ns) +
						  ((target._s || !target._ns) ? _s % target._s : 0) *
							  1000000000));
	}

	constexpr duration operator%(int64_t target) const {
		return from_s_and_ns_counts(
			0, (static_cast<int64_t>(_ns) + _s % target * 1000000000) % target);
	}

	duration &operator%=(duration target) {
		return *this = *this % target;
	}

	duration &operator%=(int64_t target) {
		return *this = *this % target;
	}

protected:
	int64_t _s;
	int32_t _ns;

	constexpr explicit duration(int64_t s_count, int32_t ns_count) :
		_s(s_count), _ns(ns_count) {}

	struct _exa_div_s {};
	struct _div_s {};
	struct _mul_ns {};

	using _in_attr = conditional_t<
		Multiple >= 1000000000,
		conditional_t<Multiple % 1000000000 == 0, _exa_div_s, _div_s>,
		_mul_ns>;

	using _out_attr = conditional_t<Multiple >= 1000000000, _div_s, _mul_ns>;

	constexpr explicit duration(_exa_div_s &&, int64_t c) :
		duration(c * (Multiple / 1000000000), 0) {}

	constexpr explicit duration(_div_s &&, int64_t c) :
		duration(
			c * (Multiple / 1000000000),
			static_cast<int32_t>(c * (Multiple % 1000000000))) {}

	static constexpr int64_t _c(_div_s &&, int64_t s, int32_t ns) {
		return s / (Multiple / 1000000000) + ns / Multiple;
	}

	constexpr explicit duration(_mul_ns &&, int64_t c) :
		duration(_mul_ns{}, c, c / (1000000000 / Multiple)) {}

	constexpr explicit duration(_mul_ns &&, int64_t c, int64_t s) :
		duration(
			s,
			static_cast<int32_t>(
				(c - s * (1000000000 / Multiple)) * Multiple)) {}

	static constexpr int64_t _c(_mul_ns &&, int64_t s, int32_t ns) {
		return s * (1000000000 / Multiple) + ns / Multiple;
	}

	constexpr int64_t _div_ns_count(int64_t ns_count) const {
		return ns_count >= 1000000000
				   ? _s / (ns_count / 1000000000) + _ns / ns_count
				   : _s * (1000000000 / ns_count) + _ns / ns_count;
	}
};

template <int64_t Multiple>
RUA_FORCE_INLINE constexpr duration<Multiple>
operator*(int64_t a, duration<Multiple> b) {
	return b * a;
}

using ns = duration<1>;
using us = duration<1000 * ns::multiple>;
using ms = duration<1000 * us::multiple>;
using s = duration<1000 * ms::multiple>;
using m = duration<60 * s::multiple>;
using h = duration<60 * m::multiple>;
using d = duration<24 * h::multiple>;
using w = duration<7 * d::multiple>;
using y = duration<365 * d::multiple>;
using ly = duration<366 * d::multiple>;

RUA_FORCE_INLINE constexpr ms duration_max() {
	return ms::from_s_and_extra_ns_counts(nmax<int64_t>(), 999999999);
}

RUA_FORCE_INLINE constexpr ms duration_zero() {
	return ms();
}

RUA_FORCE_INLINE constexpr ms duration_min() {
	return ms::from_s_and_extra_ns_counts(nmin<int64_t>(), -999999999);
}

namespace duration_literals {

RUA_FORCE_INLINE constexpr ns operator""_ns(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr us operator""_us(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr ms operator""_ms(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr s operator""_s(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr m operator""_m(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr h operator""_h(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr d operator""_d(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr w operator""_w(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr y operator""_y(unsigned long long count) {
	return static_cast<int64_t>(count);
}

RUA_FORCE_INLINE constexpr ly operator""_ly(unsigned long long count) {
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
inline void _duration_to_string_fmt_frac(
	char *buf, int w, ns v, int prec, int *nw, ns *nv) {
	// Omit trailing zeros up to and including decimal point.
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
inline int _duration_to_string_fmt_int(char *buf, int w, ns v) {
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
// second format use a smaller unit (milli-, micro-, or ns) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
inline std::string to_string(ns dur) {
	// Largest time is 2540400h10m10.000000000s
	char buf[32];
	int buf_sz = static_cast<int>(sizeof(buf));
	auto w = buf_sz;

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
			// print ns
			prec = 0;
			buf[w] = 'n';
		} else if (u < 1_ms) {
			// print us
			prec = 3;
			// U+00B5 'µ' micro sign == 0xC2 0xB5
			w--; // Need room for two bytes.
			memcpy(buf + w, "µ", str_len("µ"));
		} else {
			// print ms
			prec = 6;
			buf[w] = 'm';
		}
		_duration_to_string_fmt_frac(buf, w, u, prec, &w, &u);
		w = _duration_to_string_fmt_int(buf, w, u);
	} else {
		w--;
		buf[w] = 's';

		_duration_to_string_fmt_frac(buf, w, u, 9, &w, &u);

		// u is now integer s
		w = _duration_to_string_fmt_int(buf, w, u % 60);
		u /= 60;

		// u is now integer m
		if (u > 0) {
			w--;
			buf[w] = 'm';
			w = _duration_to_string_fmt_int(buf, w, u % 60);
			u /= 60;

			// u is now integer h
			// Stop at h because d can be different lengths.
			if (u > 0) {
				w--;
				buf[w] = 'h';
				w = _duration_to_string_fmt_int(buf, w, u);
			}
		}
	}

	if (neg) {
		w--;
		buf[w] = '-';
	}

	return std::string(buf + w, buf_sz - w);
}

} // namespace rua

#endif
