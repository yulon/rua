#include <rua/chrono.hpp>

#include <doctest/doctest.h>

using namespace rua::duration_literals;

TEST_CASE("precision of duration") {
	auto dur = rua::h(rua::d(6_ns));
	REQUIRE(dur != 0);
	REQUIRE(dur.count() == 0);
	dur /= 3;
	REQUIRE(dur != 0);
	REQUIRE(dur.count() == 0);
	REQUIRE(rua::ns(dur) == 2);
}

TEST_CASE("duration to string") {
	REQUIRE(rua::to_string(7_s + 8_ms + 2_h) == "2h0m7.008s");
}

#include <ctime>

#ifdef _MSC_VER
#define _gmtime(tm, t) gmtime_s(tm, t)
#else
#define _gmtime(tm, t) (*tm = *gmtime(t))
#endif

TEST_CASE("date convert") {
	time_t c_ti = 0;
	tm c_tm;
	_gmtime(&c_tm, &c_ti);

	rua::date_t c_epo{static_cast<int16_t>(c_tm.tm_year + 1900),
					  static_cast<signed char>(c_tm.tm_mon + 1),
					  static_cast<signed char>(c_tm.tm_mday),
					  static_cast<signed char>(c_tm.tm_hour),
					  static_cast<signed char>(c_tm.tm_min),
					  static_cast<signed char>(c_tm.tm_sec),
					  0,
					  0};

	c_ti = time(nullptr);
	_gmtime(&c_tm, &c_ti);

	auto t = rua::time(rua::s(c_ti), c_epo);
	auto d = t.date(0);

	REQUIRE(d.year == (c_tm.tm_year + 1900));
	REQUIRE(d.month == (c_tm.tm_mon + 1));
	REQUIRE(d.day == c_tm.tm_mday);
	REQUIRE(d.hour == c_tm.tm_hour);
	REQUIRE(d.minute == c_tm.tm_min);
	REQUIRE(d.second == c_tm.tm_sec);

	rua::date_t epo{1995, 12, 14, 0, 0, 0, 0, 9};
	auto d2 = rua::time(d, epo).date(0);

	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);
}
