#include <rua/bytes.hpp>
#include <rua/chrono.hpp>
#include <rua/log.hpp>
#include <rua/string.hpp>

#include <doctest/doctest.h>

TEST_CASE("memory find") {
	size_t dat_sz = 1024 * 1024 *
#ifdef NDEBUG
					256
#else
					10
#endif
		;

	std::string dat_str(dat_sz, -1);

	rua::string_view dat_sv(dat_str);
	REQUIRE(dat_sv.size() == dat_sz);

	rua::bytes_ref dat(dat_str);
	REQUIRE(dat.size() == dat_sz);

	char pat[]{-1, -1, -1, -1, -1, 6, 7, -1, 0};

	auto pat_pos = dat_str.length() - 100;

	dat(pat_pos).copy_from(&pat[0]);

	// bytes::index_of(bytes_view)

	auto tp = rua::tick();

	auto fp =
		dat.index_of(rua::bytes_view({255, 255, 255, 255, 255, 6, 7, 255}));

	rua::log("bytes::index_of(bytes_view):", rua::tick() - tp);

	REQUIRE(fp != rua::nullpos);
	REQUIRE(fp == pat_pos);

	// bytes::index_of(bytes_pattern)

	tp = rua::tick();

	fp = dat.index_of({255, 255, 255, 255, 255, 6, 7, 255});

	rua::log("bytes::index_of(bytes_pattern):", rua::tick() - tp);

	REQUIRE(fp != rua::nullpos);
	REQUIRE(fp == pat_pos);

	// bytes::index_of(masked_bytes_pattern)

	tp = rua::tick();

	fp = dat.index_of({255, 1111, 255, 255, 255, 6, 7, 255});

	rua::log("bytes::index_of(masked_bytes_pattern):", rua::tick() - tp);

	REQUIRE(fp != rua::nullpos);
	REQUIRE(fp == pat_pos);

	// bytes::find(bytes_view)

	tp = rua::tick();

	auto fr = dat.find(rua::bytes_view({255, 255, 255, 255, 255, 6, 7, 255}));

	rua::log("bytes::find(bytes_view):", rua::tick() - tp);

	REQUIRE(fr);
	REQUIRE(fr.pos() == pat_pos);
	REQUIRE(fr->size() == 8);

	// bytes::find(bytes_pattern)

	tp = rua::tick();

	fr = dat.find({255, 255, 255, 255, 255, 6, 7, 255});

	rua::log("bytes::find(bytes_pattern):", rua::tick() - tp);

	REQUIRE(fr);
	REQUIRE(fr.pos() == pat_pos);
	REQUIRE(fr->size() == 8);

	// bytes::find(masked_bytes_pattern)

	tp = rua::tick();

	fr = dat.find({255, 1111, 255, 255, 255, 6, 7, 255});

	rua::log("bytes::find(masked_bytes_pattern):", rua::tick() - tp);

	REQUIRE(fr);
	REQUIRE(fr.pos() == pat_pos);
	REQUIRE(fr->size() == 8);
	REQUIRE(fr[0].data() - fr->data() == 1);
	REQUIRE(fr[0].data() - dat.data() == pat_pos + 1);
	REQUIRE(fr[0].size() == 1);

	// std::string::find

	tp = rua::tick();

	fp = dat_str.find(pat);

	rua::log("std::string::find:", rua::tick() - tp);

	REQUIRE(fp != std::string::npos);
	REQUIRE(fp == pat_pos);

	// string_view

	tp = rua::tick();

	fp = dat_sv.find(pat);

	rua::log("string_view::find:", rua::tick() - tp);

	REQUIRE(fp != static_cast<size_t>(-1));
	REQUIRE(fp == pat_pos);
}
