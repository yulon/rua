#include <rua/chan.hpp>
#include <rua/fiber.hpp>
#include <rua/thread.hpp>

#include <catch.hpp>

#include <string>

TEST_CASE("fiber_driver", "[fiber]") {
	static rua::fiber_driver fib_dvr;
	static auto &sch = fib_dvr.get_scheduler();
	static std::string r;

	fib_dvr.attach([]() {
		r += "1";
		sch.sleep(300);
		r += "1";
	});
	fib_dvr.attach([]() {
		r += "2";
		sch.sleep(200);
		r += "2";
	});
	fib_dvr.attach([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});
	fib_dvr.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber", "[fiber]") {
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

TEST_CASE("use chan on fiber", "[fiber]") {
	rua::fiber([]() {
		rua::chan<std::string> ch;

		rua::thread([ch]() mutable {
			rua::sleep(100);
			ch << "ok";
		});

		REQUIRE(ch.get() == "ok");
	});
}
