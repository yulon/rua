#include <rua/gnc.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

TEST(Bytes, FindEfficiency) {
	size_t dat_sz = 1024 * 1024 *
		#ifdef NDEBUG
			256
		#else
			10
		#endif
	;

	std::string dat_str(dat_sz, -1);

	rua::data dat(dat_str.data(), dat_str.length());

	char pat[]{ -1, -1, -1, -1, -1, 6, 7, -1, 0 };

	auto pat_pos = dat_str.length() - 100;

	dat[pat_pos].copy(rua::data(&pat, sizeof(pat) - 1));

	// bytes::operation::find

	auto tp = std::chrono::steady_clock::now();

	auto fr = dat.find({ 255, 255, 255, 255, 255, 6, 7, 255 });

	auto data_find_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count();

	ASSERT_EQ(fr, true);
	ASSERT_EQ(fr.pos, pat_pos);

	// bytes::operation::match

	tp = std::chrono::steady_clock::now();

	auto mr = dat.match({ 255, 1111, 255, 255, 255, 6, 7, 255, 255 });

	auto data_match_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count();

	ASSERT_EQ(mr, true);
	ASSERT_EQ(mr.pos, pat_pos);
	ASSERT_EQ(mr[0], pat_pos + 1);

	// std::string::find

	tp = std::chrono::steady_clock::now();

	auto fp = dat_str.find(pat);

	auto str_find_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count();

	ASSERT_EQ(fp != std::string::npos, true);
	ASSERT_EQ(fp, pat_pos);

	std::cout <<
		"bytes::operation::find: " << data_find_dur << " ms" << std::endl <<
		"bytes::operation::match: " << data_match_dur << " ms" << std::endl <<
		"std::string::find: " << str_find_dur << " ms" << std::endl
	;
}
