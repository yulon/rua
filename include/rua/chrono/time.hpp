#ifndef _RUA_CHRONO_TIME_HPP
#define _RUA_CHRONO_TIME_HPP

#include "duration.hpp"

#include "../interface_ptr.hpp"

#include <cassert>
#include <cstdint>
#include <functional>

namespace rua {

struct date_t {
	int16_t year;
	signed char month;
	signed char day;
	signed char hour;
	signed char minute;
	signed char second;
	int32_t nanoseconds;
	signed char zone;
};

inline bool operator==(const date_t &a, const date_t &b) {
	return &a == &b ||
		   (a.year == b.year && a.month == b.month && a.day == b.day &&
			a.hour == b.hour && a.minute == b.minute && a.second == b.second &&
			a.nanoseconds == b.nanoseconds);
}

inline bool operator!=(const date_t &a, const date_t &b) {
	return !(a == b);
}

static const date_t unix_epoch{1970, 1, 1, 0, 0, 0, 0, 0};

inline bool is_leap_year(int16_t yr) {
	return !(yr % 4) && (yr % 100 || !(yr % 400));
}

static constexpr int16_t _yr_days_at_mon[]{
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31};

class time {
public:
	constexpr time() : _ela(), _zon(0), _epo(nullptr) {}

	constexpr explicit time(s elapsed, int8_t zone = 0) :
		_ela(elapsed),
		_zon(zone),
		_epo(nullptr) {}

	constexpr explicit time(s elapsed, const date_t &epoch) :
		_ela(elapsed),
		_zon(0),
		_epo(&epoch) {}

	time(s elapsed, date_t &&epoch) = delete;

	constexpr explicit time(s elapsed, int8_t zone, const date_t &epoch) :
		_ela(elapsed),
		_zon(zone),
		_epo(&epoch) {}

	time(s elapsed, int8_t zone, date_t &&epoch) = delete;

	time(const date_t &d8, const date_t &epoch = unix_epoch) :
		_zon(d8.zone),
		_epo(&epoch) {
		if (d8 == epoch) {
			return;
		}
		s secs;
		if (d8.year != _epo->year) {
			int16_t y_begin, y_end;
			if (d8.year > _epo->year) {
				y_begin = _epo->year;
				y_end = d8.year;
			} else {
				y_begin = d8.year;
				y_end = _epo->year;
			}
			int16_t fst_ly = 0, lst_ly = 0;
			for (int16_t i = y_begin; i < y_end; ++i) {
				if (is_leap_year(i)) {
					fst_ly = i;
					break;
				}
			}
			for (int16_t i = y_end - 1; i >= y_begin; --i) {
				if (is_leap_year(i)) {
					lst_ly = i;
					break;
				}
			}
			auto lys = (lst_ly - fst_ly) / 4 + 1;
			secs += y(y_end - y_begin - lys) + ly(lys);
		}
		secs += _date2dur_exc_yr(d8) - _date2dur_exc_yr(*_epo);
		secs -= h(d8.zone - _epo->zone);
		_ela = secs;
	}

	time(const date_t &d8, date_t &&epoch) = delete;

	explicit operator bool() const {
		return _epo || _ela;
	}

	s &elapsed() {
		return _ela;
	}

	s elapsed() const {
		return _ela;
	}

	int8_t &zone() {
		return _zon;
	}

	int8_t zone() const {
		return _zon;
	}

	bool is_monotonic() const {
		return !_epo;
	}

	const date_t &epoch() const {
		return *_epo;
	}

	date_t date() const {
		assert(!is_monotonic());

		date_t nd;

		// zone
		nd.zone = _zon;

		d ela_days(_ela + _date2dur_exc_yr(*_epo) - h(_epo->zone - _zon));

		// nanosecond
		nd.nanoseconds = ela_days.extra_ns_count();

		// year
		nd.year = _epo->year;

		constexpr auto days_per_400_yrs = 400 * 1_y + 97_d;
		auto ela_400_yrs = ela_days / days_per_400_yrs;
		if (ela_400_yrs) {
			ela_days %= days_per_400_yrs;
			nd.year += static_cast<int16_t>(400 * ela_400_yrs);
		}

		constexpr auto days_per_100_yrs = 100 * 1_y + 24_d;
		auto ela_100_yrs = ela_days / days_per_100_yrs;
		if (ela_100_yrs) {
			ela_100_yrs -= ela_100_yrs >> 2;
			ela_days -= days_per_100_yrs * ela_100_yrs;
			nd.year += static_cast<int16_t>(100 * ela_100_yrs);
		}

		constexpr auto days_per_4_yrs = 4 * 1_y + 1_d;
		auto ela_4_yrs = ela_days / days_per_4_yrs;
		if (ela_4_yrs) {
			ela_days %= days_per_4_yrs;
			nd.year += static_cast<int16_t>(4 * ela_4_yrs);
		}

		auto ela_yrs = ela_days / 1_y;
		if (ela_yrs) {
			ela_yrs -= ela_yrs >> 2;
			ela_days -= ela_yrs * 1_y;
			nd.year += static_cast<int16_t>(ela_yrs);
		}

		// month
		auto is_leap = is_leap_year(nd.year);
		for (size_t i = 0; i < sizeof(_yr_days_at_mon); ++i) {
			auto ydam = _yr_days_at_mon[i];
			if (i > 0 && is_leap) {
				++ydam;
			}
			if (ela_days <= ydam) {
				nd.month = static_cast<int8_t>(i + 1);
				if (i > 0) {
					auto ydabm = _yr_days_at_mon[i - 1];
					if (i > 1 && is_leap) {
						++ydabm;
					}
					ela_days -= ydabm;
				}
				break;
			}
		}

		// day
		nd.day = 1 + static_cast<int8_t>(ela_days.count());
		ela_days %= 1_d;

		// hour
		nd.hour = static_cast<int8_t>(ela_days / 1_h);
		ela_days %= 1_h;

		// minute
		nd.minute = static_cast<int8_t>(ela_days / 1_m);

		// second
		nd.second = static_cast<int8_t>((ela_days % 1_m).s_count());

		return nd;
	}

	time to_unix() const {
		assert(!is_monotonic());

		if (*_epo == unix_epoch) {
			return *this;
		}
		return time(date());
	}

	void reset() {
		_ela = 0;
		_epo = nullptr;
	}

	bool operator==(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela == target.elapsed();
		}
		return _ela == time(target.date(), *_epo).elapsed();
	}

	bool operator>(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela > target.elapsed();
		}
		return _ela > time(target.date(), *_epo).elapsed();
	}

	bool operator>=(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela >= target.elapsed();
		}
		return _ela >= time(target.date(), *_epo).elapsed();
	}

	bool operator<(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela < target.elapsed();
		}
		return _ela < time(target.date(), *_epo).elapsed();
	}

	bool operator<=(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela <= target.elapsed();
		}
		return _ela <= time(target.date(), *_epo).elapsed();
	}

	time operator+(s dur) const {
		return time(_ela + dur, *_epo);
	}

	time &operator+=(s dur) {
		_ela += dur;
		return *this;
	}

	time operator-(s dur) const {
		return time(_ela - dur, *_epo);
	}

	s operator-(const time &target) const {
		if (!_epo || target.is_monotonic() || *_epo == target.epoch()) {
			return _ela - target.elapsed();
		}
		return _ela - time(target.date(), *_epo).elapsed();
	}

	s operator-(const date_t &d8) const {
		return _ela - time(d8, *_epo).elapsed();
	}

	time &operator-=(s dur) {
		_ela -= dur;
		return *this;
	}

private:
	s _ela;
	int8_t _zon;
	const date_t *_epo;

	static duration _date2dur_exc_yr(const date_t &d8) {
		duration r;
		if (d8.month > 1) {
			r += d(_yr_days_at_mon[d8.month - 2]);
			if (d8.month > 2 && is_leap_year(d8.year)) {
				r += 1_d;
			}
		}
		r += d(d8.day - 1) + h(d8.hour) + m(d8.minute) + s(d8.second) +
			 ns(d8.nanoseconds);
		return r;
	}
};

RUA_FORCE_INLINE time operator+(s dur, const time &tim) {
	return tim + dur;
}

RUA_FORCE_INLINE time time_max() {
	return time(duration_max());
}

RUA_FORCE_INLINE time time_max(const date_t &epoch) {
	return time(duration_max(), epoch);
}

RUA_FORCE_INLINE time time_zero() {
	return time(duration_zero());
}

RUA_FORCE_INLINE time time_zero(const date_t &epoch) {
	return time(duration_zero(), epoch);
}

RUA_FORCE_INLINE time time_min() {
	return time(duration_min());
}

RUA_FORCE_INLINE time time_min(const date_t &epoch) {
	return time(duration_min(), epoch);
}

} // namespace rua

namespace std {

template <>
struct hash<rua::time> {
	RUA_FORCE_INLINE size_t operator()(const rua::time &t) const {
		return _szt(
			t.is_monotonic() ? rua::ms(t.elapsed()).count()
							 : t.to_unix().elapsed().s_count());
	}

private:
	static RUA_FORCE_INLINE size_t _szt(int64_t c) {
		return static_cast<size_t>(c >= 0 ? c : 0);
	}
};

} // namespace std

#endif
