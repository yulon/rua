#include <rua/fiber.hpp>
#include <rua/thread.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("fiber_runer run") {
	static rua::fiber_runer fr;
	static auto &dzr = fr.get_dozer();
	static std::string r;

	fr.add([]() {
		r += "1";
		dzr.sleep(300);
		r += "1";
	});
	fr.add([]() {
		r += "2";
		dzr.sleep(200);
		r += "2";
	});
	fr.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	fr.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber_runer step") {
	static rua::fiber_runer fr;
	static auto &dzr = fr.get_dozer();
	static std::string r;

	fr.add([]() {
		r += "1";
		dzr.sleep(300);
		r += "1";
	});
	fr.add([]() {
		r += "2";
		dzr.sleep(200);
		r += "2";
	});
	fr.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	while (fr) {
		fr.step();
		rua::sleep(50);
	}

	REQUIRE(r == "123321");
}

TEST_CASE("fiber long-lasting task") {
	static rua::fiber_runer fr;
	static auto &dzr = fr.get_dozer();
	static size_t c = 0;

	fr.add([]() { ++c; }, rua::duration_max());

	for (size_t i = 0; i < 5; ++i) {
		fr.step();
	}

	REQUIRE(c == 5);
}

TEST_CASE("fiber wake sequence") {
	static rua::fiber_runer fr;
	static auto &dzr = fr.get_dozer();
	static std::string r;

	fr.add([]() { r += "1"; });
	fr.add([]() { r += "2"; });
	fr.add([]() { r += "3"; });

	fr.step();

	REQUIRE(r == "123");

	r.resize(0);

	fr.add([]() {
		r += "1";
		dzr.sleep(100);
		r += "1";
	});
	fr.add([]() {
		r += "2";
		dzr.sleep(100);
		r += "2";
	});
	fr.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	fr.step();
	rua::sleep(200);
	fr.step();

	REQUIRE(r == "123123");
}

TEST_CASE("co") {
	static std::string r;

	rua::co([]() {
		rua::co([]() {
			r += "1";
			rua::sleep(300);
			r += "1";
		});
		rua::co([]() {
			r += "2";
			rua::sleep(200);
			r += "2";
		});
		rua::co([]() {
			r += "3";
			rua::sleep(100);
			r += "3";
		});
	});

	REQUIRE(r == "123321");
}

TEST_CASE("use chan on fiber") {
	rua::co([]() {
		static rua::chan<std::string> ch;

		rua::thread([]() mutable {
			rua::sleep(100);
			ch << "ok";
		});

		REQUIRE(ch.pop() == "ok");
	});
}
