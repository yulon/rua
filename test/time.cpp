#include <rua/time.hpp>

#include <doctest/doctest.h>

using namespace rua::duration_literals;

TEST_CASE("duration to string") {
	CHECK(rua::to_string(7_s + 8_ms + 2_h) == "2h0m7.008s");
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

	CHECK(d.year == (c_tm.tm_year + 1900));
	CHECK(d.month == (c_tm.tm_mon + 1));
	CHECK(d.day == c_tm.tm_mday);
	CHECK(d.hour == c_tm.tm_hour);
	CHECK(d.minute == c_tm.tm_min);
	CHECK(d.second == c_tm.tm_sec);

	rua::date epo{1995, 12, 14, 0, 0, 0, 0, 8};
	auto d2 = rua::time(d, epo).point();

	CHECK(d.year == d2.year);
	CHECK(d.month == d2.month);
	CHECK(d.day == d2.day);
	CHECK(d.hour == d2.hour);
	CHECK(d.minute == d2.minute);
	CHECK(d.second == d2.second);
	CHECK(d.nanoseconds == d2.nanoseconds);

	auto ts = 1638318469 + rua::hours(8).seconds();
	d = {2021, 12, 1, 8, 27, 49};

	t = rua::time(rua::seconds(ts), 0, rua::unix_epoch);
	d2 = t.point();
	CHECK(t.elapsed().seconds() == ts);
	CHECK(d.year == d2.year);
	CHECK(d.month == d2.month);
	CHECK(d.day == d2.day);
	CHECK(d.hour == d2.hour);
	CHECK(d.minute == d2.minute);
	CHECK(d.second == d2.second);
	CHECK(d.nanoseconds == d2.nanoseconds);

	t = rua::time(d, rua::unix_epoch);
	d2 = t.point();
	CHECK(t.elapsed().seconds() == ts);
	CHECK(d.year == d2.year);
	CHECK(d.month == d2.month);
	CHECK(d.day == d2.day);
	CHECK(d.hour == d2.hour);
	CHECK(d.minute == d2.minute);
	CHECK(d.second == d2.second);
	CHECK(d.nanoseconds == d2.nanoseconds);
}
