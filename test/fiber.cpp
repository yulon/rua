#include <rua/fiber.hpp>
#include <rua/sync.hpp>
#include <rua/thread.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("fiber_driver run") {
	static rua::fiber_driver dvr;
	static auto &sch = dvr.get_scheduler();
	static std::string r;

	dvr.attach([]() {
		r += "1";
		sch.sleep(300);
		r += "1";
	});
	dvr.attach([]() {
		r += "2";
		sch.sleep(200);
		r += "2";
	});
	dvr.attach([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});
	dvr.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber_driver step") {
	static rua::fiber_driver dvr;
	static auto &sch = dvr.get_scheduler();
	static std::string r;

	dvr.attach([]() {
		r += "1";
		sch.sleep(300);
		r += "1";
	});
	dvr.attach([]() {
		r += "2";
		sch.sleep(200);
		r += "2";
	});
	dvr.attach([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});

	while (dvr) {
		dvr.step();
		rua::sleep(50);
	}

	REQUIRE(r == "123321");

	static size_t c = 0;

	dvr.attach([]() { ++c; }, rua::duration_max());

	for (size_t i = 0; i < 5; ++i) {
		dvr.step();
	}

	REQUIRE(c == 5);
}

TEST_CASE("fiber") {
	static std::string r;

	rua::fiber([]() {
		rua::fiber([]() {
			r += "1";
			rua::sleep(300);
			r += "1";
		});
		rua::fiber([]() {
			r += "2";
			rua::sleep(200);
			r += "2";
		});
		rua::fiber([]() {
			r += "3";
			rua::sleep(100);
			r += "3";
		});
	});

	REQUIRE(r == "123321");
}

TEST_CASE("use chan on fiber") {
	rua::fiber([]() {
		static rua::chan<std::string> ch;

		rua::thread([]() mutable {
			rua::sleep(100);
			ch << "ok";
		});

		REQUIRE(ch.pop() == "ok");
	});
}
