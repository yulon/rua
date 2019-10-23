#ifndef _RUA_CHRONO_TIME_HPP
#define _RUA_CHRONO_TIME_HPP

#include "duration.hpp"
#include "zone.hpp"

#include "../ref.hpp"

#include <cassert>
#include <cstdint>
#include <functional>

namespace rua {

struct date_t {
	int16_t year;
	int8_t month;
	int8_t day;
	int8_t hour;
	int8_t minute;
	int8_t second;
	int32_t nanoseconds;
	int8_t zone;
};

inline bool operator==(const date_t &a, const date_t &b) {
	return a.year == b.year && a.month == b.month && a.day == b.day &&
		   a.hour == b.hour && a.minute == b.minute && a.second == b.second &&
		   a.nanoseconds == b.nanoseconds;
}

inline bool operator!=(const date_t &a, const date_t &b) {
	return !(a == b);
}

static const date_t unix_start_date{
	1970,
	1,
	1,
	0,
	0,
	0,
	0,
	0,
};

inline bool is_leap_year(int16_t yr) {
	return !(yr % 4) && (yr % 100 || !(yr % 400));
}

static constexpr int16_t _days_before_month[]{
	0,
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
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31,
};

class time {
public:
	constexpr time() : _ela(), _start(nullptr) {}

	time(duration_base elapsed, ref<const date_t> start_date = nullptr) :
		_ela(elapsed),
		_start(std::move(start_date)) {}

	time(const date_t &end, ref<const date_t> start_date = unix_start_date) :
		_start(std::move(start_date)) {

		s secs;
		if (end.year != _start->year) {
			int16_t y_begin, y_end;
			if (end.year > _start->year) {
				y_begin = _start->year;
				y_end = end.year;
			} else {
				y_begin = end.year;
				y_end = _start->year;
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
		secs += _date2s_exc_yr(end) - _date2s_exc_yr(*_start);
		secs -= h(end.zone - _start->zone);
		_ela = duration_base(secs.count(), end.nanoseconds);
	}

	explicit operator bool() const {
		return _start || _ela;
	}

	template <RUA_DURATION_CONCEPT(Duration)>
	constexpr Duration elapsed() const {
		return Duration(_ela);
	}

	duration_base elapsed() const {
		return _ela;
	}

	bool has_start_date() const {
		return _start;
	}

	ref<const date_t> start_date() const {
		return _start;
	}

	date_t date(int8_t zone = local_time_zone()) const {
		assert(has_start_date());

		date_t nd;

		// zone
		nd.zone = zone;

		auto ela_s = _ela.total_seconds() + _date2s_exc_yr(*_start) -
					 h(_start->zone - zone);

		// nanosecond
		ns ns_sum = _ela.extra_nanoseconds() + _start->nanoseconds;
		ela_s += ns_sum.total_seconds();
		nd.nanoseconds = ns_sum.extra_nanoseconds();

		auto ela_days = d(ela_s);

		// year
		nd.year = _start->year;

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
		auto end_year_is_leap = is_leap_year(nd.year);
		for (int i = 11; i >= 0; --i) {
			auto dbm = _days_before_month[i];
			if (i >= 1 && end_year_is_leap) {
				++dbm;
			}
			if (dbm < ela_days) {
				nd.month = static_cast<int8_t>(i + 1);
				ela_days = ela_days - dbm + 1;
				break;
			}
		}

		// day
		nd.day = static_cast<int8_t>(ela_days.count());
		ela_s %= 1_d;

		// hour
		nd.hour = static_cast<int8_t>(ela_s / 1_h);
		ela_s %= 1_h;

		// minute
		nd.minute = static_cast<int8_t>(ela_s / 1_m);

		// second
		nd.second = static_cast<int8_t>((ela_s % 1_m).count());

		return nd;
	}

	time to_unix() const {
		assert(has_start_date());

		if (*_start == unix_start_date) {
			return *this;
		}
		return time(date(0));
	}

	void reset() {
		_ela = duration_base();
		_start = nullptr;
	}

	bool operator==(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela == target.elapsed();
		}
		return _ela == time(target.date(), _start).elapsed();
	}

	bool operator>(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela > target.elapsed();
		}
		return _ela > time(target.date(), _start).elapsed();
	}

	bool operator>=(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela >= target.elapsed();
		}
		return _ela >= time(target.date(), _start).elapsed();
	}

	bool operator<(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela < target.elapsed();
		}
		return _ela < time(target.date(), _start).elapsed();
	}

	bool operator<=(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela <= target.elapsed();
		}
		return _ela <= time(target.date(), _start).elapsed();
	}

	time operator+(duration_base dur) const {
		return time(_ela + dur, _start);
	}

	time &operator+=(duration_base dur) {
		_ela += dur;
		return *this;
	}

	time operator-(duration_base dur) const {
		return time(_ela - dur, _start);
	}

	duration_base operator-(const time &target) const {
		if (!_start || !target.has_start_date() ||
			*_start == *target.start_date()) {
			return _ela - target.elapsed();
		}
		return _ela - time(target.date(), _start).elapsed();
	}

	duration_base operator-(const date_t &d8) const {
		return _ela - time(d8, _start).elapsed();
	}

	time &operator-=(duration_base dur) {
		_ela -= dur;
		return *this;
	}

private:
	duration_base _ela;
	ref<const date_t> _start;

	static s _date2s_exc_yr(const date_t &d8) {
		auto r = s(d(_days_before_month[d8.month - 1]));
		if ((d8.month > 1 && is_leap_year(d8.year)) ||
			(d8.month == 1 && d8.day == 29)) {
			r += 1_d;
		}
		r += d(d8.day - 1) + h(d8.hour) + m(d8.minute) + s(d8.second);
		return r;
	}
};

RUA_FORCE_INLINE time operator+(duration_base dur, const time &tim) {
	return tim + dur;
}

RUA_FORCE_INLINE time time_max(ref<const date_t> start_date = nullptr) {
	return time(duration_max(), std::move(start_date));
}

RUA_FORCE_INLINE time time_zero(ref<const date_t> start_date = nullptr) {
	return time(duration_zero(), std::move(start_date));
}

RUA_FORCE_INLINE time time_min(ref<const date_t> start_date = nullptr) {
	return time(duration_min(), std::move(start_date));
}

} // namespace rua

namespace std {

template <>
struct hash<rua::time> {
	RUA_FORCE_INLINE size_t operator()(const rua::time &t) const {
		return _szt(
			t.has_start_date() ? t.to_unix().elapsed().total_seconds()
							   : rua::ms(t.elapsed()).count());
	}

private:
	static RUA_FORCE_INLINE size_t _szt(int64_t c) {
		return static_cast<size_t>(c >= 0 ? c : 0);
	}
};

template <>
class less<rua::time> {
public:
	bool operator()(const rua::time &lhs, const rua::time &rhs) const {
		return lhs < rhs;
	}
};

} // namespace std

#endif
