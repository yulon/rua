#include <rua/bin.hpp>
#include <rua/chrono.hpp>
#include <rua/console.hpp>

#include <catch.hpp>

#include <iostream>

TEST_CASE("find", "[bin]") {
	size_t dat_sz = 1024 * 1024 *
#ifdef NDEBUG
					256
#else
					10
#endif
		;

	std::string dat_str(dat_sz, -1);

	rua::bin_ref dat(dat_str);

	REQUIRE(dat.size() == dat_sz);

	char pat[]{-1, -1, -1, -1, -1, 6, 7, -1, 0};

	auto pat_pos = dat_str.length() - 100;

	dat(pat_pos).copy(&pat[0]);

	// bin_base::find

	auto tp = rua::tick();

	auto fr = dat.find({255, 255, 255, 255, 255, 6, 7, 255});

	auto find_dur = rua::tick() - tp;

	REQUIRE(fr);
	REQUIRE(fr.pos == pat_pos);

	// bin_base::match

	tp = rua::tick();

	auto mr = dat.match({255, 1111, 255, 255, 255, 6, 7, 255});

	auto match_dur = rua::tick() - tp;

	REQUIRE(mr);
	REQUIRE(mr.pos == pat_pos);
	REQUIRE(mr[0] == pat_pos + 1);

	// std::string::find

	tp = rua::tick();

	auto fp = dat_str.find(pat);

	auto str_find_dur = rua::tick() - tp;

	REQUIRE(fp != std::string::npos);
	REQUIRE(fp == pat_pos);

	rua::clog("bin_base::find:", find_dur);
	rua::clog("bin_base::match:", match_dur);
	rua::clog("std::string::find:", str_find_dur);
}
