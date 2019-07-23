#include <rua/bin.hpp>

#include <catch.hpp>

#include <chrono>
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

	auto tp = std::chrono::steady_clock::now();

	auto fr = dat.find({255, 255, 255, 255, 255, 6, 7, 255});

	auto data_find_dur = std::chrono::duration_cast<std::chrono::milliseconds>(
							 std::chrono::steady_clock::now() - tp)
							 .count();

	REQUIRE(fr);
	REQUIRE(fr.pos == pat_pos);

	// bin_base::match

	tp = std::chrono::steady_clock::now();

	auto mr = dat.match({255, 1111, 255, 255, 255, 6, 7, 255});

	auto data_match_dur = std::chrono::duration_cast<std::chrono::milliseconds>(
							  std::chrono::steady_clock::now() - tp)
							  .count();

	REQUIRE(mr);
	REQUIRE(mr.pos == pat_pos);
	REQUIRE(mr[0] == pat_pos + 1);

	// std::string::find

	tp = std::chrono::steady_clock::now();

	auto fp = dat_str.find(pat);

	auto str_find_dur = std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - tp)
							.count();

	REQUIRE(fp != std::string::npos);
	REQUIRE(fp == pat_pos);

	std::cout << "bin_base::find: " << data_find_dur << " ms" << std::endl
			  << "bin_base::match: " << data_match_dur << " ms" << std::endl
			  << "std::string::find: " << str_find_dur << " ms" << std::endl;
}
