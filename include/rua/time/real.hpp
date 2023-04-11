#ifndef _rua_time_real_hpp
#define _rua_time_real_hpp

#include "duration.hpp"

#include "../util.hpp"

#include <cassert>
#include <functional>

namespace rua {

struct date {
	int16_t year;
	signed char month;
	signed char day;
	signed char hour;
	signed char minute;
	signed char second;
	int32_t nanoseconds;
	signed char zone;
};

inline bool operator==(const date &a, const date &b) {
	return &a == &b ||
		   (a.year == b.year && a.month == b.month && a.day == b.day &&
			a.hour == b.hour && a.minute == b.minute && a.second == b.second &&
			a.nanoseconds == b.nanoseconds);
}

inline bool operator!=(const date &a, const date &b) {
	return !(a == b);
}

inline bool operator>(const date &a, const date &b) {
	return &a != &b && (a.year > b.year || a.month > b.month || a.day > b.day ||
						a.hour > b.hour || a.minute > b.minute ||
						a.second > b.second || a.nanoseconds > b.nanoseconds);
}

inline bool operator>=(const date &a, const date &b) {
	return a == b || a > b;
}

inline bool operator<(const date &a, const date &b) {
	return &a != &b && (a.year < b.year || a.month < b.month || a.day < b.day ||
						a.hour < b.hour || a.minute < b.minute ||
						a.second < b.second || a.nanoseconds < b.nanoseconds);
}

inline bool operator<=(const date &a, const date &b) {
	return a == b || a < b;
}

RUA_CVAR const date nullepo{0, 0, 0, 0, 0, 0, 0, 0};
RUA_CVAR const date unix_epoch{1970, 1, 1, 0, 0, 0, 0, 0};

inline constexpr bool is_leap_year(int16_t yr) {
	return !(yr % 4) && ((yr % 100) || !(yr % 400));
}

RUA_CVAL int16_t _yr_days_at_mon_tab[]{
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

inline constexpr int16_t _yr_days_at_mon(bool is_ly, size_t m) {
	return m > 1 && is_ly ? _yr_days_at_mon_tab[m] + 1 : _yr_days_at_mon_tab[m];
}

class time {
public:
	constexpr time() : $ela(), $zon(0), $epo(&nullepo) {}

	constexpr time(duration elapsed, int8_t zone = 0) :
		$ela(elapsed), $zon(zone), $epo(&unix_epoch) {}

	constexpr time(duration elapsed, const date &epoch) :
		$ela(elapsed), $zon(0), $epo(&epoch) {}

	time(duration elapsed, date &&epoch) = delete;

	constexpr time(duration elapsed, int8_t zone, const date &epoch) :
		$ela(elapsed), $zon(zone), $epo(&epoch) {}

	time(duration elapsed, int8_t zone, date &&epoch) = delete;

	time(const date &d8, const date &epoch = unix_epoch) :
		$ela(), $zon(d8.zone), $epo(&epoch) {
		if (d8 == epoch) {
			return;
		}
		$ela = days($days_b4_yr(d8.year) - $days_b4_yr($epo->year)) +
			   $d8_wo_yr_to_dur(d8) - $d8_wo_yr_to_dur(*$epo) - hours(d8.zone);
	}

	time(const date &d8, date &&epoch) = delete;

	bool is_monotonic() const {
		return *$epo == nullepo;
	}

	explicit operator bool() const {
		return !is_monotonic() || $ela;
	}

	const date &epoch() const {
		return *$epo;
	}

	duration &elapsed() {
		return $ela;
	}

	duration elapsed() const {
		return $ela;
	}

	int8_t &zone() {
		return $zon;
	}

	int8_t zone() const {
		return $zon;
	}

	date point() const {
		assert(!is_monotonic());

		date nd;

		// zone
		nd.zone = $zon;

		auto ela = $ela + $d8_wo_yr_to_dur(*$epo) + hours($zon);

		// nanosecond
		nd.nanoseconds = $ela.remaining_nanoseconds();

		auto ela_days = ela.days();

		// year
		nd.year = 1;

		ela_days += $days_b4_yr($epo->year);

		constexpr int64_t days_per_400_years = 365 * 400 + 97;
		auto n = ela_days / days_per_400_years;
		nd.year += static_cast<uint16_t>(400 * n);
		ela_days -= days_per_400_years * n;

		constexpr int64_t days_per_100_years = 365 * 100 + 24;
		n = ela_days / days_per_100_years;
		n -= n >> 2;
		nd.year += static_cast<uint16_t>(100 * n);
		ela_days -= days_per_100_years * n;

		constexpr int64_t days_per_4_years = 365 * 4 + 1;
		n = ela_days / days_per_4_years;
		nd.year += static_cast<uint16_t>(4 * n);
		ela_days -= days_per_4_years * n;

		n = ela_days / 365;
		n -= n >> 2;
		nd.year += static_cast<uint16_t>(n);
		ela_days -= 365 * n;

		// month
		auto is_ly = is_leap_year(nd.year);
		for (size_t i = 12; i > 0; --i) {
			nd.month = static_cast<int8_t>(i);
			auto ydam = _yr_days_at_mon(is_ly, i - 1);
			if (ydam <= ela_days) {
				ela_days -= ydam;
				break;
			}
		}

		// day
		++ela_days;
		nd.day = static_cast<int8_t>(ela_days);

		// hour
		ela %= day;
		nd.hour = static_cast<int8_t>(ela / hour);

		// minute
		ela %= hour;
		nd.minute = static_cast<int8_t>(ela / minute);

		// second
		nd.second = static_cast<int8_t>((ela % minute).seconds());

		return nd;
	}

	time to(const date &target_epoch) const {
		assert(!is_monotonic());

		if (*$epo == target_epoch) {
			return *this;
		}
		return time(point(), target_epoch);
	}

	time to_unix() const {
		return to(unix_epoch);
	}

	void reset() {
		$ela = 0;
		$epo = &nullepo;
	}

	bool operator==(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela == target.elapsed();
		}
		return $ela == time(target.point(), *$epo).elapsed();
	}

	bool operator>(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela > target.elapsed();
		}
		return $ela > time(target.point(), *$epo).elapsed();
	}

	bool operator>=(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela >= target.elapsed();
		}
		return $ela >= time(target.point(), *$epo).elapsed();
	}

	bool operator<(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela < target.elapsed();
		}
		return $ela < time(target.point(), *$epo).elapsed();
	}

	bool operator<=(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela <= target.elapsed();
		}
		return $ela <= time(target.point(), *$epo).elapsed();
	}

	time operator+(duration dur) const {
		return time($ela + dur, *$epo);
	}

	time &operator+=(duration dur) {
		$ela += dur;
		return *this;
	}

	time operator-(duration dur) const {
		return time($ela - dur, *$epo);
	}

	duration operator-(const time &target) const {
		if (is_monotonic() || target.is_monotonic() ||
			*$epo == target.epoch()) {
			return $ela - target.elapsed();
		}
		return $ela - time(target.point(), *$epo).elapsed();
	}

	duration operator-(const date &d8) const {
		return $ela - time(d8, *$epo).elapsed();
	}

	time &operator-=(duration dur) {
		$ela -= dur;
		return *this;
	}

private:
	duration $ela;
	int8_t $zon;
	const date *$epo;

	static RUA_CONSTEXPR_14 int64_t $days_b4_yr(uint16_t yr) {
		--yr;
		return yr * 365 + yr / 4 - yr / 100 + yr / 400;
	}

	static duration $d8_wo_yr_to_dur(const date &d8) {
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

} // namespace rua

namespace std {

template <>
struct hash<rua::time> {
	size_t operator()(const rua::time &t) const {
		return $szt(
			t.is_monotonic() ? t.elapsed().milliseconds()
							 : t.to_unix().elapsed().seconds());
	}

private:
	static size_t $szt(int64_t c) {
		return static_cast<size_t>(c >= 0 ? c : 0);
	}
};

} // namespace std

#endif
