#include <rua/fiber.hpp>
#include <rua/thread.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("fiber_executor run") {
	static rua::fiber_executor exr;
	static auto &spdr = exr.get_suspender();
	static std::string r;

	exr.execute([]() {
		r += "1";
		spdr.sleep(300);
		r += "1";
	});
	exr.execute([]() {
		r += "2";
		spdr.sleep(200);
		r += "2";
	});
	exr.execute([]() {
		r += "3";
		spdr.sleep(100);
		r += "3";
	});

	exr.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber_executor step") {
	static rua::fiber_executor exr;
	static auto &spdr = exr.get_suspender();
	static std::string r;

	exr.execute([]() {
		r += "1";
		spdr.sleep(300);
		r += "1";
	});
	exr.execute([]() {
		r += "2";
		spdr.sleep(200);
		r += "2";
	});
	exr.execute([]() {
		r += "3";
		spdr.sleep(100);
		r += "3";
	});

	while (exr) {
		exr.step();
		rua::sleep(50);
	}

	REQUIRE(r == "123321");
}

TEST_CASE("fiber long-lasting task") {
	static rua::fiber_executor exr;
	static auto &spdr = exr.get_suspender();
	static size_t c = 0;

	exr.execute([]() { ++c; }, rua::duration_max());

	for (size_t i = 0; i < 5; ++i) {
		exr.step();
	}

	REQUIRE(c == 5);
}

TEST_CASE("fiber resume sequence") {
	static rua::fiber_executor exr;
	static auto &spdr = exr.get_suspender();
	static std::string r;

	exr.execute([]() { r += "1"; });
	exr.execute([]() { r += "2"; });
	exr.execute([]() { r += "3"; });

	exr.step();

	REQUIRE(r == "123");

	r.resize(0);

	exr.execute([]() {
		r += "1";
		spdr.sleep(100);
		r += "1";
	});
	exr.execute([]() {
		r += "2";
		spdr.sleep(100);
		r += "2";
	});
	exr.execute([]() {
		r += "3";
		spdr.sleep(100);
		r += "3";
	});

	exr.step();
	rua::sleep(200);
	exr.step();

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
