#ifndef _RUA_CHRONO_TIME_HPP
#define _RUA_CHRONO_TIME_HPP

#include "duration.hpp"

#include "../interface_ptr.hpp"
#include "../types/util.hpp"

#include <cassert>
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

inline bool operator>(const date_t &a, const date_t &b) {
	return &a != &b && (a.year > b.year || a.month > b.month || a.day > b.day ||
						a.hour > b.hour || a.minute > b.minute ||
						a.second > b.second || a.nanoseconds > b.nanoseconds);
}

inline bool operator>=(const date_t &a, const date_t &b) {
	return a == b || a > b;
}

inline bool operator<(const date_t &a, const date_t &b) {
	return &a != &b && (a.year < b.year || a.month < b.month || a.day < b.day ||
						a.hour < b.hour || a.minute < b.minute ||
						a.second < b.second || a.nanoseconds < b.nanoseconds);
}

inline bool operator<=(const date_t &a, const date_t &b) {
	return a == b || a < b;
}

RUA_MULTIDEF_VAR const date_t nullepo{0, 0, 0, 0, 0, 0, 0, 0};
RUA_MULTIDEF_VAR const date_t unix_epoch{1970, 1, 1, 0, 0, 0, 0, 0};

inline uchar is_leap_year(int16_t yr) {
	if (yr % 4) {
		return 0;
	}
	if (yr % 100) {
		if (yr % 400) {
			return 2;
		}
		return 0;
	}
	return 1;
}

inline int16_t _yr_days_at_mon(bool is_ly, size_t m) {
	static const int16_t tab[]{
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
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31};
	return m > 1 && is_ly ? tab[m] + 1 : tab[m];
}

class time {
public:
	constexpr time() : _ela(), _zon(0), _epo(&nullepo) {}

	constexpr time(duration elapsed, int8_t zone = 0) :
		_ela(elapsed), _zon(zone), _epo(&unix_epoch) {}

	constexpr time(duration elapsed, const date_t &epoch) :
		_ela(elapsed), _zon(0), _epo(&epoch) {}

	time(duration elapsed, date_t &&epoch) = delete;

	constexpr time(duration elapsed, int8_t zone, const date_t &epoch) :
		_ela(elapsed), _zon(zone), _epo(&epoch) {}

	time(duration elapsed, int8_t zone, date_t &&epoch) = delete;

	time(const date_t &d8, const date_t &epoch = unix_epoch) :
		_ela(), _zon(d8.zone), _epo(&epoch) {
		if (d8 == epoch) {
			return;
		}
		if (d8.year != _epo->year) {
			int16_t y_begin, y_end;
			if (d8.year > _epo->year) {
				y_begin = _epo->year;
				y_end = d8.year;
			} else {
				y_begin = d8.year;
				y_end = _epo->year;
			}
			int16_t fst_qy = 0, lst_qy = 0;
			for (int16_t i = y_begin; i < y_end; ++i) {
				if (!(i % 4)) {
					fst_qy = i;
					break;
				}
			}
			for (int16_t i = y_end - 1; i >= y_begin; --i) {
				if (!(i % 4)) {
					lst_qy = i;
					break;
				}
			}
			auto qy_diff = lst_qy - fst_qy;
			auto yrs = years(y_end - y_begin) +
					   days(
						   qy_diff / 4 + 1 - (qy_diff / 100 + 1) +
						   (qy_diff / 400 + 1));
			if (d8.year > _epo->year) {
				_ela += yrs;
			} else {
				_ela -= yrs;
			}
		}
		_ela += _date2dur_exc_yr(d8);
		_ela -= _date2dur_exc_yr(*_epo);
		_ela -= hours(d8.zone);
	}

	time(const date_t &d8, date_t &&epoch) = delete;

	bool is_monotonic() const {
		return *_epo == nullepo;
	}

	explicit operator bool() const {
		return !is_monotonic() || _ela;
	}

	const date_t &epoch() const {
		return *_epo;
	}

	duration &elapsed() {
		return _ela;
	}

	duration elapsed() const {
		return _ela;
	}

	int8_t &zone() {
		return _zon;
	}

	int8_t zone() const {
		return _zon;
	}

	date_t date() const {
		assert(!is_monotonic());

		date_t nd;

		// zone
		nd.zone = _zon;

		auto ela = _ela + _date2dur_exc_yr(*_epo) + hours(_zon);

		// nanosecond
		nd.nanoseconds = _ela.remaining_nanoseconds();

		// year
		nd.year = _epo->year;
		for (;;) {
			auto yr_dur = is_leap_year(nd.year) ? leep_year : year;
			if (ela < yr_dur) {
				break;
			}
			ela -= yr_dur;
			++nd.year;
		}

		// month
		auto is_leap = is_leap_year(nd.year);
		for (size_t i = 12; i > 0; --i) {
			nd.month = static_cast<int8_t>(i);
			auto ydam = _yr_days_at_mon(is_leap, i - 1);
			if (ydam < ela.days()) {
				ela -= days(ydam);
				break;
			}
		}

		// day
		nd.day = 1 + static_cast<int8_t>(ela.days());
		ela %= day;

		// hour
		nd.hour = static_cast<int8_t>(ela / hour);
		ela %= hour;

		// minute
		nd.minute = static_cast<int8_t>(ela / minute);

		// second
		nd.second = static_cast<int8_t>((ela % minute).seconds());

		return nd;
	}

	time to(const date_t &target_epoch) const {
		assert(!is_monotonic());

		if (*_epo == target_epoch) {
			return *this;
		}
		return time(date(), target_epoch);
	}

	time to_unix() const {
		return to(unix_epoch);
	}

	void reset() {
		_ela = 0;
		_epo = &nullepo;
	}

	bool operator==(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela == target.elapsed();
		}
		return _ela == time(target.date(), *_epo).elapsed();
	}

	bool operator>(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela > target.elapsed();
		}
		return _ela > time(target.date(), *_epo).elapsed();
	}

	bool operator>=(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela >= target.elapsed();
		}
		return _ela >= time(target.date(), *_epo).elapsed();
	}

	bool operator<(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela < target.elapsed();
		}
		return _ela < time(target.date(), *_epo).elapsed();
	}

	bool operator<=(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela <= target.elapsed();
		}
		return _ela <= time(target.date(), *_epo).elapsed();
	}

	time operator+(duration dur) const {
		return time(_ela + dur, *_epo);
	}

	time &operator+=(duration dur) {
		_ela += dur;
		return *this;
	}

	time operator-(duration dur) const {
		return time(_ela - dur, *_epo);
	}

	duration operator-(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*_epo == target.epoch()) {
			return _ela - target.elapsed();
		}
		return _ela - time(target.date(), *_epo).elapsed();
	}

	duration operator-(const date_t &d8) const {
		return _ela - time(d8, *_epo).elapsed();
	}

	time &operator-=(duration dur) {
		_ela -= dur;
		return *this;
	}

private:
	duration _ela;
	int8_t _zon;
	const date_t *_epo;

	static duration _date2dur_exc_yr(const date_t &d8) {
		duration r;
		if (d8.month > 1) {
			r += days(_yr_days_at_mon(is_leap_year(d8.year), d8.month - 1));
		}
		r += days(d8.day - 1) + hours(d8.hour) + minutes(d8.minute) +
			 seconds(d8.second) + nanoseconds(d8.nanoseconds);
		return r;
	}
};

inline time operator+(duration dur, const time &tim) {
	return tim + dur;
}

inline time time_max() {
	return time(duration_max());
}

inline time time_max(const date_t &epoch) {
	return time(duration_max(), epoch);
}

inline time time_zero() {
	return time(duration_zero());
}

inline time time_zero(const date_t &epoch) {
	return time(duration_zero(), epoch);
}

inline time time_min() {
	return time(duration_min());
}

inline time time_min(const date_t &epoch) {
	return time(duration_min(), epoch);
}

} // namespace rua

namespace std {

template <>
struct hash<rua::time> {
	size_t operator()(const rua::time &t) const {
		return _szt(
			t.is_monotonic() ? t.elapsed().milliseconds()
							 : t.to_unix().elapsed().seconds());
	}

private:
	static size_t _szt(int64_t c) {
		return static_cast<size_t>(c >= 0 ? c : 0);
	}
};

} // namespace std

#endif
