#include <rua/time.hpp>

#include <doctest/doctest.h>

using namespace rua::duration_literals;

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

	rua::date c_epo{
		static_cast<int16_t>(c_tm.tm_year + 1900),
		static_cast<signed char>(c_tm.tm_mon + 1),
		static_cast<signed char>(c_tm.tm_mday),
		static_cast<signed char>(c_tm.tm_hour),
		static_cast<signed char>(c_tm.tm_min),
		static_cast<signed char>(c_tm.tm_sec),
		0,
		0};

	c_ti = time(nullptr);
	_gmtime(&c_tm, &c_ti);

	auto t = rua::time(rua::seconds(c_ti), c_epo);
	auto d = t.point();

	REQUIRE(d.year == (c_tm.tm_year + 1900));
	REQUIRE(d.month == (c_tm.tm_mon + 1));
	REQUIRE(d.day == c_tm.tm_mday);
	REQUIRE(d.hour == c_tm.tm_hour);
	REQUIRE(d.minute == c_tm.tm_min);
	REQUIRE(d.second == c_tm.tm_sec);

	rua::date epo{1995, 12, 14, 0, 0, 0, 0, 8};
	auto d2 = rua::time(d, epo).point();

	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);

	auto ts = 1596008104;
	d = {2020, 7, 29, 15, 35, 04, 0, 8};
	t = rua::time(d);
	d2 = t.point();

	REQUIRE(t.elapsed().seconds() == ts);
	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);

	t = rua::time(rua::seconds(ts), 8);
	d2 = t.point();
	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);
}
