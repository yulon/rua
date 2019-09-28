#ifndef _RUA_CHRONO_DURATION_HPP
#define _RUA_CHRONO_DURATION_HPP

#include "../bytes.hpp"
#include "../limits.hpp"
#include "../macros.hpp"
#include "../string/strlen.hpp"
#include "../string/to_string.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace rua {

class duration_base {
public:
	constexpr explicit duration_base(int64_t secs = 0, int32_t nanos = 0) :
		_s(secs),
		_n(nanos) {}

#ifdef RUA_CONSTEXPR_14_SUPPORTED

	static constexpr duration_base make(int64_t secs, int64_t nanos) {
		auto carrys = nanos / 1000000000;
		if (carrys != 0) {
			secs += carrys;
			nanos %= 1000000000;
		}
		if (secs < 0 && nanos > 0) {
			++secs;
			nanos = -(1000000000 - nanos);
		} else if (secs > 0 && nanos < 0) {
			--secs;
			nanos = 1000000000 + nanos;
		}
		return duration_base(secs, static_cast<int32_t>(nanos));
	}

#else

private:
	static RUA_FORCE_INLINE constexpr duration_base
	_handle_pan(int64_t secs, int64_t nanos) {
		return (secs < 0 && nanos > 0)
				   ? duration_base(
						 secs + 1, static_cast<int32_t>(-(1000000000 - nanos)))
				   : ((secs > 0 && nanos < 0)
						  ? duration_base(
								secs - 1,
								static_cast<int32_t>(1000000000 + nanos))
						  : duration_base(secs, static_cast<int32_t>(nanos)));
	}

	static RUA_FORCE_INLINE constexpr duration_base _handle_overflow(
		int64_t secs, int64_t nanos, int64_t carrys, int64_t remainders) {
		return (carrys != 0) ? _handle_pan(secs + carrys, remainders)
							 : _handle_pan(secs, nanos);
	}

public:
	static constexpr duration_base make(int64_t secs, int64_t nanos) {
		return _handle_overflow(
			secs, nanos, nanos / 1000000000, nanos % 1000000000);
	}

#endif

	constexpr int64_t total_seconds() const {
		return _s;
	}

	constexpr int32_t extra_nanoseconds() const {
		return _n;
	}

	constexpr explicit operator bool() const {
		return _s || _n;
	}

	constexpr bool operator!() const {
		return !_s && !_n;
	}

	constexpr bool operator==(duration_base target) const {
		return _s == target._s && _n == target._n;
	}

	constexpr bool operator!=(duration_base target) const {
		return _s != target._s || _n != target._n;
	}

	constexpr bool operator>(duration_base target) const {
		return _s > target._s || (_s == target._s && _n > target._n);
	}

	constexpr bool operator<(duration_base target) const {
		return _s < target._s || (_s == target._s && _n < target._n);
	}

	constexpr bool operator>=(duration_base target) const {
		return _s >= target._s || (_s == target._s && _n >= target._n);
	}

	constexpr bool operator<=(duration_base target) const {
		return _s <= target._s || (_s == target._s && _n <= target._n);
	}

	constexpr duration_base operator+(duration_base target) const {
		return make(
			_s + target._s,
			static_cast<int64_t>(_n) + static_cast<int64_t>(target._n));
	}

	duration_base &operator+=(duration_base target) {
		return *this = *this + target;
	}

	constexpr duration_base operator-(duration_base target) const {
		return make(
			_s - target._s,
			static_cast<int64_t>(_n) - static_cast<int64_t>(target._n));
	}

	constexpr duration_base operator-() const {
		return duration_base(-_s, -_n);
	}

	duration_base &operator-=(duration_base target) {
		return *this = *this - target;
	}

	constexpr duration_base operator*(int64_t target) const {
		return make(_s * target, static_cast<int64_t>(_n) * target);
	}

	duration_base &operator*=(int64_t target) {
		return *this = *this * target;
	}

	constexpr int64_t operator/(duration_base target) const {
		return ((target._s || !target._n) ? _s / target._s : 0) +
			   (target._n ? ((static_cast<int64_t>(_n) +
							  _s % target._s * 1000000000) /
							 static_cast<int64_t>(target._n))
						  : 0);
	}

	constexpr duration_base operator/(int64_t target) const {
		return make(
			_s / target,
			(static_cast<int64_t>(_n) + _s % target * 1000000000) / target);
	}

	duration_base &operator/=(int64_t target) {
		return *this = *this / target;
	}

	constexpr duration_base operator%(duration_base target) const {
		return make(
			0,
			target._n ? (static_cast<int64_t>(_n) +
						 ((target._s || !target._n) ? _s % target._s : 0) *
							 1000000000) %
							target._n
					  : (static_cast<int64_t>(_n) +
						 ((target._s || !target._n) ? _s % target._s : 0) *
							 1000000000));
	}

	constexpr duration_base operator%(int64_t target) const {
		return make(
			0, (static_cast<int64_t>(_n) + _s % target * 1000000000) % target);
	}

	duration_base &operator%=(duration_base target) {
		return *this = *this % target;
	}

	duration_base &operator%=(int64_t target) {
		return *this = *this % target;
	}

protected:
	int64_t _s;
	int32_t _n;

	void _fix() {
		auto new_secs = _n / 1000000000;
		if (new_secs > 0) {
			_s += new_secs;
			_n %= 1000000000;
		}
		if ((_s < 0 && _n > 0) || (_s > 0 && _n < 0)) {
			++_s;
			_n = -(1000000000 - _n);
		}
	}
};

RUA_FORCE_INLINE constexpr duration_base operator*(int64_t a, duration_base b) {
	return b * a;
}

RUA_FORCE_INLINE constexpr duration_base duration_max() {
	return duration_base(nmax<int64_t>(), 999999999);
}

RUA_FORCE_INLINE constexpr duration_base duration_zero() {
	return duration_base();
}

RUA_FORCE_INLINE constexpr duration_base duration_min() {
	return duration_base(nmin<int64_t>(), -999999999);
}

template <
	int64_t Multiple,
	typename = typename std::enable_if<(Multiple > 0)>::type>
class duration : public duration_base {
public:
	static constexpr int64_t multiple = Multiple;

	constexpr duration(int64_t count = 0) :
		duration_base(_in(count, _in_attr{})) {}

	constexpr duration(duration_base other_dur) : duration_base(other_dur) {}

	constexpr int64_t count() const {
		return _out(_s, _n, _out_attr{});
	}

	duration &operator+=(duration target) {
		*static_cast<duration_base *>(this) += target;
		return *this;
	}

	duration &operator-=(duration target) {
		*static_cast<duration_base *>(this) -= target;
		return *this;
	}

	duration &operator*=(int64_t target) {
		*static_cast<duration_base *>(this) *= target;
		return *this;
	}

	duration &operator/=(int64_t target) {
		*static_cast<duration_base *>(this) /= target;
		return *this;
	}

	duration &operator%=(duration target) {
		*static_cast<duration_base *>(this) %= target;
		return *this;
	}

	duration &operator++() {
		return *this += 1;
	}

	duration operator++(int) {
		auto old = *this;
		++*this;
		return old;
	}

	duration &operator--() {
		*this -= 1;
	}

	duration operator--(int) {
		auto old = *this;
		--*this;
		return old;
	}

private:
	struct _exa_div_s {};
	struct _div_s {};
	struct _mul_n {};

	using _in_attr = typename std::conditional<
		Multiple >= 1000000000,
		typename std::
			conditional<Multiple % 1000000000 == 0, _exa_div_s, _div_s>::type,
		_mul_n>::type;

	using _out_attr =
		typename std::conditional<Multiple >= 1000000000, _div_s, _mul_n>::type;

	static constexpr duration_base _in(int64_t c, _exa_div_s &&) {
		return duration_base(c * (Multiple / 1000000000));
	}

	static constexpr duration_base _in(int64_t c, _div_s &&) {
		return duration_base(
			c * (Multiple / 1000000000),
			static_cast<int32_t>(c * (Multiple % 1000000000)));
	}

	static constexpr int64_t _out(int64_t s, int32_t n, _div_s &&) {
		return s / (Multiple / 1000000000) + n / Multiple;
	}

	static constexpr duration_base _in(int64_t c, _mul_n &&) {
		return _make_for_mul_n(c, c / (1000000000 / Multiple));
	}

	static constexpr duration_base _make_for_mul_n(int64_t c, int64_t s) {
		return duration_base(
			s,
			static_cast<int32_t>((c - s * (1000000000 / Multiple)) * Multiple));
	}

	static constexpr int64_t _out(int64_t s, int32_t n, _mul_n &&) {
		return s * (1000000000 / Multiple) + n / Multiple;
	}
};

#define RUA_DURATION_CONCEPT(Duration)                                         \
	typename Duration,                                                         \
		typename = typename std::enable_if <                                   \
					   std::is_base_of<duration_base, Duration>::value &&      \
				   !std::is_same<duration_base, Duration>::value > ::type

#define RUA_DURATION_EXPR_CONCEPT(First, Second, FirstDuration)                \
	typename First, typename Second,                                           \
                                                                               \
		typename = typename std::enable_if <                                   \
					   (std::is_base_of<duration_base, First>::value &&        \
						!std::is_same<duration_base, First>::value &&          \
						std::is_convertible<Second, First>::value) ||          \
				   (std::is_base_of<duration_base, Second>::value &&           \
					!std::is_same<duration_base, Second>::value &&             \
					std::is_convertible<First, Second>::value) > ::type,       \
                                                                               \
		typename FirstDuration = typename std::conditional<                    \
			(std::is_base_of<duration_base, First>::value &&                   \
			 !std::is_same<duration_base, First>::value &&                     \
			 std::is_convertible<Second, First>::value),                       \
			First,                                                             \
			Second>::type

#define RUA_DURATION_PAIR_CONCEPT(FirstDuration, SecondDuration)               \
	typename FirstDuration, typename SecondDuration,                           \
                                                                               \
		typename =                                                             \
			typename std::enable_if <                                          \
				(std::is_base_of<duration_base, FirstDuration>::value &&       \
				 !std::is_same<duration_base, FirstDuration>::value &&         \
				 std::is_base_of<duration_base, SecondDuration>::value) ||     \
			(std::is_base_of<duration_base, SecondDuration>::value &&          \
			 !std::is_same<duration_base, SecondDuration>::value &&            \
			 std::is_base_of<duration_base, FirstDuration>::value) > ::type

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator==(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) ==
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator!=(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) !=
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator>(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) >
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator<(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) <
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator>=(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) >=
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr bool operator<=(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) <=
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr Dur operator+(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) +
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_EXPR_CONCEPT(A, B, Dur)>
RUA_FORCE_INLINE constexpr Dur operator-(A a, B b) {
	return static_cast<duration_base &&>(Dur(a)) -
		   static_cast<duration_base &&>(Dur(b));
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr Dur operator*(Dur a, int64_t b) {
	return static_cast<duration_base &>(a) * b;
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr Dur operator*(int64_t a, Dur b) {
	return a * static_cast<duration_base &>(b);
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr Dur operator/(Dur a, int64_t b) {
	return static_cast<duration_base &>(a) / b;
}

template <RUA_DURATION_PAIR_CONCEPT(DurA, DurB)>
RUA_FORCE_INLINE constexpr DurA operator%(DurA a, DurB b) {
	return static_cast<duration_base &>(a) % static_cast<duration_base &>(b);
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr Dur operator%(Dur a, int64_t b) {
	return static_cast<duration_base &>(a) % b;
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr Dur duration_max() {
	return Dur(duration_max());
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr duration_base duration_zero() {
	return Dur(duration_zero());
}

template <RUA_DURATION_CONCEPT(Dur)>
RUA_FORCE_INLINE constexpr duration_base duration_min() {
	return Dur(duration_min());
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
	char *buf, size_t w, ns v, int prec, int *nw, ns *nv) {
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
inline int _duration_to_string_fmt_int(char *buf, size_t w, ns v) {
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
			memcpy(buf + w, "µ", strlen("µ"));
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
