#include <rua/chrono.hpp>

#include <catch.hpp>

using namespace rua::duration_literals;

TEST_CASE("precision of duration", "[chrono]") {
	auto dur = rua::h(rua::d(6_ns));
	REQUIRE(dur != 0);
	REQUIRE(dur.count() == 0);
	dur /= 3;
	REQUIRE(dur != 0);
	REQUIRE(dur.count() == 0);
	REQUIRE(rua::ns(dur) == 2);
}

TEST_CASE("duration to string", "[chrono]") {
	REQUIRE(rua::to_string(7_s + 8_ms + 2_h) == "2h0m7.008s");
}

TEST_CASE("date convert", "[chrono]") {
	auto t = rua::now();
	auto d = t.end();
	auto d2 = rua::time(
				  d,
				  rua::date{
					  1995,
					  12,
					  14,
					  0,
					  0,
					  0,
					  0,
					  9,
				  })
				  .end();

	REQUIRE(d.year == d2.year);
	REQUIRE(d.month == d2.month);
	REQUIRE(d.day == d2.day);
	REQUIRE(d.hour == d2.hour);
	REQUIRE(d.minute == d2.minute);
	REQUIRE(d.second == d2.second);
	REQUIRE(d.nanoseconds == d2.nanoseconds);
}

#include <ctime>

TEST_CASE("now", "[chrono]") {
	auto c_ti = time(nullptr);
	auto c_tm = *gmtime(&c_ti);

	auto d = rua::now().end(0);

	auto c_ti_2 = time(nullptr);
	auto c_tm_2 = *gmtime(&c_ti_2);

	if (c_tm_2.tm_year == c_tm.tm_year) {
		REQUIRE(d.year == (c_tm.tm_year + 1900));
	}
	if (c_tm_2.tm_mon == c_tm.tm_mon) {
		REQUIRE(d.month == (c_tm.tm_mon + 1));
	}
	if (c_tm_2.tm_mday == c_tm.tm_mday) {
		REQUIRE(d.day == c_tm.tm_mday);
	}
	if (c_tm_2.tm_hour == c_tm.tm_hour) {
		REQUIRE(d.hour == c_tm.tm_hour);
	}
	if (c_tm_2.tm_min == c_tm.tm_min) {
		REQUIRE(d.minute == c_tm.tm_min);
	}
	if (c_tm_2.tm_sec == c_tm.tm_sec) {
		REQUIRE(d.second == c_tm.tm_sec);
	}
}
