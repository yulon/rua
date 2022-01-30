#ifndef _RUA_TIME_DURATION_HPP
#define _RUA_TIME_DURATION_HPP

#include "../bytes.hpp"
#include "../macros.hpp"
#include "../string/conv.hpp"
#include "../string/len.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <cstring>
#include <ctime>

namespace rua {

namespace multiple_of_nanosecond {

RUA_CVAL int64_t nanosecond = 1;
RUA_CVAL int64_t microsecond = 1000 * nanosecond;
RUA_CVAL int64_t millisecond = 1000 * microsecond;
RUA_CVAL int64_t second = 1000 * millisecond;
RUA_CVAL int64_t minute = 60 * second;
RUA_CVAL int64_t hour = 60 * minute;
RUA_CVAL int64_t day = 24 * hour;
RUA_CVAL int64_t week = 7 * day;
RUA_CVAL int64_t year = 365 * day;
RUA_CVAL int64_t leep_year = 366 * day;

} // namespace multiple_of_nanosecond

class duration {
private:
	template <int64_t MultipleOfNanosecond>
	struct _mul_s {};

	template <int64_t MultipleOfNanosecond>
	struct _ge_s {};

	template <int64_t MultipleOfNanosecond>
	struct _ovf_ns {};

	template <int64_t MultipleOfNanosecond>
	using _in_attr = conditional_t<
		MultipleOfNanosecond >= multiple_of_nanosecond::second,
		conditional_t<
			MultipleOfNanosecond % multiple_of_nanosecond::second == 0,
			_mul_s<MultipleOfNanosecond>,
			_ge_s<MultipleOfNanosecond>>,
		_ovf_ns<MultipleOfNanosecond>>;

	template <int64_t MultipleOfNanosecond>
	using _out_attr = conditional_t<
		MultipleOfNanosecond >= multiple_of_nanosecond::second,
		_ge_s<MultipleOfNanosecond>,
		_ovf_ns<MultipleOfNanosecond>>;

	template <int64_t MultipleOfNanosecond>
	static constexpr duration
	_from(_mul_s<MultipleOfNanosecond> &&, int64_t c) {
		return duration(
			c * (MultipleOfNanosecond / multiple_of_nanosecond::second), 0);
	}

	template <int64_t MultipleOfNanosecond>
	static constexpr duration _from(_ge_s<MultipleOfNanosecond> &&, int64_t c) {
		return duration(
			c * (MultipleOfNanosecond / multiple_of_nanosecond::second),
			static_cast<int32_t>(
				c * (MultipleOfNanosecond % multiple_of_nanosecond::second)));
	}

	template <int64_t MultipleOfNanosecond>
	constexpr int64_t _count(_ge_s<MultipleOfNanosecond> &&) const {
		return _s / (MultipleOfNanosecond / multiple_of_nanosecond::second) +
			   _ns / MultipleOfNanosecond;
	}

	template <int64_t MultipleOfNanosecond>
	static constexpr duration
	_from(_ovf_ns<MultipleOfNanosecond> &&, int64_t c) {
		return _from(
			_ovf_ns<MultipleOfNanosecond>{},
			c,
			c / (multiple_of_nanosecond::second / MultipleOfNanosecond));
	}

	template <int64_t MultipleOfNanosecond>
	static constexpr duration
	_from(_ovf_ns<MultipleOfNanosecond> &&, int64_t c, int64_t s) {
		return duration(
			s,
			static_cast<int32_t>(
				(c -
				 s * (multiple_of_nanosecond::second / MultipleOfNanosecond)) *
				MultipleOfNanosecond));
	}

	template <int64_t MultipleOfNanosecond>
	constexpr int64_t _count(_ovf_ns<MultipleOfNanosecond> &&) const {
		return _s * (multiple_of_nanosecond::second / MultipleOfNanosecond) +
			   _ns / MultipleOfNanosecond;
	}

	constexpr int64_t _div_ns_count(int64_t ns) const {
		return ns >= multiple_of_nanosecond::second
				   ? _s / (ns / multiple_of_nanosecond::second) + _ns / ns
				   : _s * (multiple_of_nanosecond::second / ns) + _ns / ns;
	}

	template <typename CountT, CountT Max>
	constexpr enable_if_t<std::is_unsigned<CountT>::value, CountT>
	_revise_count(int64_t c) const {
		return is_max() ? Max
						: (static_cast<uint64_t>(nmax<CountT>()) <
								   static_cast<uint64_t>(c)
							   ? nmax<CountT>()
							   : static_cast<CountT>(c));
	}

	template <typename CountT, CountT Max>
	constexpr enable_if_t<!std::is_unsigned<CountT>::value, CountT>
	_revise_count(int64_t c) const {
		return is_max() ? Max
						: (static_cast<int64_t>(nmax<CountT>()) < c
							   ? nmax<CountT>()
							   : static_cast<CountT>(c));
	}

#ifndef RUA_CONSTEXPR_14_SUPPORTED

	static constexpr duration
	_from_overflowable__pan(int64_t seconds, int64_t nanoseconds) {
		return (seconds < 0 && nanoseconds > 0)
				   ? duration(
						 seconds + 1,
						 static_cast<int32_t>(
							 -(multiple_of_nanosecond::second - nanoseconds)))
				   : ((seconds > 0 && nanoseconds < 0)
						  ? duration(
								seconds - 1,
								static_cast<int32_t>(
									multiple_of_nanosecond::second +
									nanoseconds))
						  : duration(
								seconds, static_cast<int32_t>(nanoseconds)));
	}

	static constexpr duration _from_overflowable__overflow(
		int64_t seconds,
		int64_t nanoseconds,
		int64_t carrys,
		int64_t remainders) {
		return (carrys != 0)
				   ? _from_overflowable__pan(seconds + carrys, remainders)
				   : _from_overflowable__pan(seconds, nanoseconds);
	}

	static constexpr duration
	_from_overflowable(int64_t seconds, int64_t nanoseconds) {
		return _from_overflowable__overflow(
			seconds,
			nanoseconds,
			nanoseconds / multiple_of_nanosecond::second,
			nanoseconds % multiple_of_nanosecond::second);
	}

#else

	static constexpr duration
	_from_overflowable(int64_t seconds, int64_t nanoseconds) {
		auto carrys = nanoseconds / multiple_of_nanosecond::second;
		if (carrys != 0) {
			seconds += carrys;
			nanoseconds %= multiple_of_nanosecond::second;
		}
		if (seconds < 0 && nanoseconds > 0) {
			++seconds;
			nanoseconds = -(multiple_of_nanosecond::second - nanoseconds);
		} else if (seconds > 0 && nanoseconds < 0) {
			--seconds;
			nanoseconds = multiple_of_nanosecond::second + nanoseconds;
		}
		return duration(seconds, static_cast<int32_t>(nanoseconds));
	}

#endif

public:
	static constexpr auto remaining_nanoseconds_max =
		multiple_of_nanosecond::second - 1;

	static constexpr auto remaining_nanoseconds_min =
		-remaining_nanoseconds_max;

	template <int64_t MultipleOfNanosecond>
	static constexpr duration from(int64_t count) {
		return _from(_in_attr<MultipleOfNanosecond>{}, count);
	}

public:
	constexpr duration() : _s(0), _ns(0) {}

	constexpr duration(int64_t milliseconds) :
		duration(_from(
			_in_attr<multiple_of_nanosecond::millisecond>{}, milliseconds)) {}

	constexpr duration(int64_t seconds, int32_t remaining_nanoseconds) :
		_s(seconds), _ns(remaining_nanoseconds) {}

	constexpr duration(const timespec &ts) : _s(ts.tv_sec), _ns(ts.tv_nsec) {}

	constexpr explicit operator bool() const {
		return _s || _ns;
	}

	constexpr bool is_max() const {
		return _s == nmax<int64_t>() && _ns == remaining_nanoseconds_max;
	}

	constexpr bool is_min() const {
		return _s == nmin<int64_t>() && _ns == remaining_nanoseconds_min;
	}

	template <int64_t MultipleOfNanosecond>
	constexpr int64_t count() const {
		return _count(_out_attr<MultipleOfNanosecond>{});
	}

	template <
		int64_t MultipleOfNanosecond,
		typename CountT,
		CountT Max = nmax<CountT>()>
	constexpr int64_t count() const {
		return _revise_count<CountT, Max>(
			_count(_out_attr<MultipleOfNanosecond>{}));
	}

	constexpr int64_t nanoseconds() const {
		return _s * multiple_of_nanosecond::second + _ns;
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT nanoseconds() const {
		return _revise_count<CountT, Max>(nanoseconds());
	}

	constexpr int64_t microseconds() const {
		return _count(_out_attr<multiple_of_nanosecond::microsecond>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT microseconds() const {
		return _revise_count<CountT, Max>(microseconds());
	}

	constexpr int64_t milliseconds() const {
		return _count(_out_attr<multiple_of_nanosecond::millisecond>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT milliseconds() const {
		return _revise_count<CountT, Max>(milliseconds());
	}

	constexpr int64_t seconds() const {
		return _s;
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT seconds() const {
		return _revise_count<CountT, Max>(_s);
	}

	constexpr int32_t remaining_nanoseconds() const {
		return _ns;
	}

	template <typename CountT>
	constexpr CountT remaining_nanoseconds() const {
		return static_cast<CountT>(_ns);
	}

	constexpr int64_t minutes() const {
		return _count(_out_attr<multiple_of_nanosecond::minute>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT minutes() const {
		return _revise_count<CountT, Max>(minutes());
	}

	constexpr int64_t hours() const {
		return _count(_out_attr<multiple_of_nanosecond::hour>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT hours() const {
		return _revise_count<CountT, Max>(hours());
	}

	constexpr int64_t days() const {
		return _count(_out_attr<multiple_of_nanosecond::day>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT days() const {
		return _revise_count<CountT, Max>(days());
	}

	constexpr int64_t weeks() const {
		return _count(_out_attr<multiple_of_nanosecond::week>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT weeks() const {
		return _revise_count<CountT, Max>(weeks());
	}

	constexpr int64_t years() const {
		return _count(_out_attr<multiple_of_nanosecond::year>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT years() const {
		return _revise_count<CountT, Max>(years());
	}

	constexpr int64_t leep_years() const {
		return _count(_out_attr<multiple_of_nanosecond::leep_year>{});
	}

	template <typename CountT, CountT Max = nmax<CountT>()>
	constexpr CountT leep_years() const {
		return _revise_count<CountT, Max>(leep_years());
	}

	constexpr timespec c_timespec() const {
		return {
			seconds<decltype(timespec::tv_sec)>(),
			remaining_nanoseconds<decltype(timespec::tv_nsec)>()};
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
		return _from_overflowable(
			_s + target._s,
			static_cast<int64_t>(_ns) + static_cast<int64_t>(target._ns));
	}

	duration &operator+=(duration target) {
		return *this = *this + target;
	}

	constexpr duration operator-(duration target) const {
		return _from_overflowable(
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
		return _from_overflowable(
			_s * target, static_cast<int64_t>(_ns) * target);
	}

	duration &operator*=(int64_t target) {
		return *this = *this * target;
	}

	int64_t operator/(duration target) const {
		return *this >= target ? _div_ns_count(target.nanoseconds()) : 0;
	}

	constexpr duration operator/(int64_t target) const {
		return _from_overflowable(
			_s / target,
			(static_cast<int64_t>(_ns) +
			 _s % target * multiple_of_nanosecond::second) /
				target);
	}

	duration &operator/=(int64_t target) {
		return *this = *this / target;
	}

	constexpr duration operator%(duration target) const {
		return _from_overflowable(
			0,
			target._ns ? (static_cast<int64_t>(_ns) +
						  ((target._s || !target._ns) ? _s % target._s : 0) *
							  multiple_of_nanosecond::second) %
							 target._ns
					   : (static_cast<int64_t>(_ns) +
						  ((target._s || !target._ns) ? _s % target._s : 0) *
							  multiple_of_nanosecond::second));
	}

	constexpr duration operator%(int64_t target) const {
		return _from_overflowable(
			0,
			(static_cast<int64_t>(_ns) +
			 _s % target * multiple_of_nanosecond::second) %
				target);
	}

	duration &operator%=(duration target) {
		return *this = *this % target;
	}

	duration &operator%=(int64_t target) {
		return *this = *this % target;
	}

private:
	int64_t _s;
	int32_t _ns;
};

inline constexpr duration operator*(int64_t a, duration b) {
	return b * a;
}

inline constexpr duration duration_max() {
	return duration(nmax<int64_t>(), duration::remaining_nanoseconds_max);
}

inline constexpr duration duration_zero() {
	return duration();
}

inline constexpr duration duration_min() {
	return duration(nmin<int64_t>(), duration::remaining_nanoseconds_min);
}

inline constexpr duration nanoseconds(int64_t count) {
	return duration::from<multiple_of_nanosecond::nanosecond>(count);
}

inline constexpr duration microseconds(int64_t count) {
	return duration::from<multiple_of_nanosecond::microsecond>(count);
}

inline constexpr duration milliseconds(int64_t count) {
	return duration::from<multiple_of_nanosecond::millisecond>(count);
}

inline constexpr duration seconds(int64_t count) {
	return duration::from<multiple_of_nanosecond::second>(count);
}

inline constexpr duration minutes(int64_t count) {
	return duration::from<multiple_of_nanosecond::minute>(count);
}

inline constexpr duration hours(int64_t count) {
	return duration::from<multiple_of_nanosecond::hour>(count);
}

inline constexpr duration days(int64_t count) {
	return duration::from<multiple_of_nanosecond::day>(count);
}

inline constexpr duration weeks(int64_t count) {
	return duration::from<multiple_of_nanosecond::week>(count);
}

inline constexpr duration years(int64_t count) {
	return duration::from<multiple_of_nanosecond::year>(count);
}

inline constexpr duration leep_years(int64_t count) {
	return duration::from<multiple_of_nanosecond::leep_year>(count);
}

namespace duration_literals {

inline constexpr duration operator""_ns(unsigned long long count) {
	return nanoseconds(static_cast<int64_t>(count));
}

inline constexpr duration operator""_us(unsigned long long count) {
	return microseconds(static_cast<int64_t>(count));
}

inline constexpr duration operator""_ms(unsigned long long count) {
	return milliseconds(static_cast<int64_t>(count));
}

inline constexpr duration operator""_s(unsigned long long count) {
	return seconds(static_cast<int64_t>(count));
}

inline constexpr duration operator""_m(unsigned long long count) {
	return minutes(static_cast<int64_t>(count));
}

inline constexpr duration operator""_h(unsigned long long count) {
	return hours(static_cast<int64_t>(count));
}

inline constexpr duration operator""_d(unsigned long long count) {
	return days(static_cast<int64_t>(count));
}

inline constexpr duration operator""_w(unsigned long long count) {
	return weeks(static_cast<int64_t>(count));
}

inline constexpr duration operator""_y(unsigned long long count) {
	return years(static_cast<int64_t>(count));
}

inline constexpr duration operator""_ly(unsigned long long count) {
	return leep_years(static_cast<int64_t>(count));
}

} // namespace duration_literals

using namespace duration_literals;

RUA_CVAL auto nanosecond = 1_ns;
RUA_CVAL auto microsecond = 1_us;
RUA_CVAL auto millisecond = 1_ms;
RUA_CVAL auto second = 1_s;
RUA_CVAL auto minute = 1_m;
RUA_CVAL auto hour = 1_h;
RUA_CVAL auto day = 1_d;
RUA_CVAL auto week = 1_w;
RUA_CVAL auto year = 1_y;
RUA_CVAL auto leep_year = 1_ly;

////////////////////////////////////////////////////////////////////////////

// Reference from
// https://github.com/golang/go/blob/release-branch.go1.12/src/time/time.go#L665

// _duration_to_string_fmt_frac formats the fraction of v/10**prec (e.g.,
// ".12345") into the tail of buf, omitting trailing zeros. It omits the decimal
// point too when the fraction is 0. It returns the index where the
// output bytes begin and the value v/10**prec.
inline void _duration_to_string_fmt_frac(
	char *buf, int w, duration v, int prec, int *nw, duration *nv) {
	// Omit trailing zeros up to and including decimal point.
	auto print = false;
	for (auto i = 0; i < prec; i++) {
		auto digit = v.nanoseconds() % 10;
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
inline int _duration_to_string_fmt_int(char *buf, int w, duration v) {
	if (v == 0) {
		w--;
		buf[w] = '0';
	} else {
		while (v > 0) {
			w--;
			buf[w] = static_cast<char>(v.nanoseconds() % 10) + '0';
			v /= 10;
		}
	}
	return w;
}

// String returns a string representing the duration in the form "72h3m0.5s".
// Leading zero units are omitted. As a special case, durations less than one
// second format use a smaller unit (milli-, micro-, or ns) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
inline std::string to_string(duration dur) {
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
			memcpy(buf + w, "µ", c_str_len("µ"));
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
