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

TEST_CASE("date convert") {
	auto t = rua::now();
	auto d = t.date();
	auto d2 = rua::time(
				  d,
				  rua::date_t{
					  1995,
					  12,
					  14,
					  0,
					  0,
					  0,
					  0,
					  9,
				  })
				  .date();

	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);
}

#include <ctime>

#ifdef _MSC_VER
#define _gmtime(tm, t) gmtime_s(tm, t)
#else
#define _gmtime(tm, t) (*tm = *gmtime(t))
#endif

TEST_CASE("now") {
	auto c_ti = time(nullptr);
	tm c_tm;
	_gmtime(&c_tm, &c_ti);

	auto d = rua::now().date(0);

	auto c_ti_2 = time(nullptr);
	tm c_tm_2;
	_gmtime(&c_tm_2, &c_ti_2);

	if (c_tm_2.tm_year == c_tm.tm_year) {
		CHECK(d.year == (c_tm.tm_year + 1900));
	}
	if (c_tm_2.tm_mon == c_tm.tm_mon) {
		CHECK(d.month == (c_tm.tm_mon + 1));
	}
	if (c_tm_2.tm_mday == c_tm.tm_mday) {
		CHECK(d.day == c_tm.tm_mday);
	}
	if (c_tm_2.tm_hour == c_tm.tm_hour) {
		CHECK(d.hour == c_tm.tm_hour);
	}
	if (c_tm_2.tm_min == c_tm.tm_min) {
		CHECK(d.minute == c_tm.tm_min);
	}
	if (c_tm_2.tm_sec == c_tm.tm_sec) {
		CHECK(d.second == c_tm.tm_sec);
	}
}
