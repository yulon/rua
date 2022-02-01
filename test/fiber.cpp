#include <rua/fiber.hpp>
#include <rua/thread.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("fiber_pool run") {
	static rua::fiber_pool fp;
	static auto &dzr = fp.get_dozer();
	static std::string r;

	fp.add([]() {
		r += "1";
		dzr.sleep(300);
		r += "1";
	});
	fp.add([]() {
		r += "2";
		dzr.sleep(200);
		r += "2";
	});
	fp.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	fp.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber_pool step") {
	static rua::fiber_pool fp;
	static auto &dzr = fp.get_dozer();
	static std::string r;

	fp.add([]() {
		r += "1";
		dzr.sleep(300);
		r += "1";
	});
	fp.add([]() {
		r += "2";
		dzr.sleep(200);
		r += "2";
	});
	fp.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	while (fp) {
		fp.step();
		rua::sleep(50);
	}

	REQUIRE(r == "123321");
}

TEST_CASE("fiber long-lasting task") {
	static rua::fiber_pool fp;
	static auto &dzr = fp.get_dozer();
	static size_t c = 0;

	fp.add([]() { ++c; }, rua::duration_max());

	for (size_t i = 0; i < 5; ++i) {
		fp.step();
	}

	REQUIRE(c == 5);
}

TEST_CASE("fiber wake sequence") {
	static rua::fiber_pool fp;
	static auto &dzr = fp.get_dozer();
	static std::string r;

	fp.add([]() { r += "1"; });
	fp.add([]() { r += "2"; });
	fp.add([]() { r += "3"; });

	fp.step();

	REQUIRE(r == "123");

	r.resize(0);

	fp.add([]() {
		r += "1";
		dzr.sleep(100);
		r += "1";
	});
	fp.add([]() {
		r += "2";
		dzr.sleep(100);
		r += "2";
	});
	fp.add([]() {
		r += "3";
		dzr.sleep(100);
		r += "3";
	});

	fp.step();
	rua::sleep(200);
	fp.step();

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
