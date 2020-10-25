#include <rua/fiber.hpp>
#include <rua/thread.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("fiber_executor run") {
	static rua::fiber_executor dvr;
	static auto &sch = dvr.get_scheduler();
	static std::string r;

	dvr.execute([]() {
		r += "1";
		sch.sleep(300);
		r += "1";
	});
	dvr.execute([]() {
		r += "2";
		sch.sleep(200);
		r += "2";
	});
	dvr.execute([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});

	dvr.run();

	REQUIRE(r == "123321");
}

TEST_CASE("fiber_executor step") {
	static rua::fiber_executor dvr;
	static auto &sch = dvr.get_scheduler();
	static std::string r;

	dvr.execute([]() {
		r += "1";
		sch.sleep(300);
		r += "1";
	});
	dvr.execute([]() {
		r += "2";
		sch.sleep(200);
		r += "2";
	});
	dvr.execute([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});

	while (dvr) {
		dvr.step();
		rua::sleep(50);
	}

	REQUIRE(r == "123321");
}

TEST_CASE("fiber long-lasting task") {
	static rua::fiber_executor dvr;
	static auto &sch = dvr.get_scheduler();
	static size_t c = 0;

	dvr.execute([]() { ++c; }, rua::duration_max());

	for (size_t i = 0; i < 5; ++i) {
		dvr.step();
	}

	REQUIRE(c == 5);
}

TEST_CASE("fiber resume sequence") {
	static rua::fiber_executor dvr;
	static auto &sch = dvr.get_scheduler();
	static std::string r;

	dvr.execute([]() { r += "1"; });
	dvr.execute([]() { r += "2"; });
	dvr.execute([]() { r += "3"; });

	dvr.step();

	REQUIRE(r == "123");

	r.resize(0);

	dvr.execute([]() {
		r += "1";
		sch.sleep(100);
		r += "1";
	});
	dvr.execute([]() {
		r += "2";
		sch.sleep(100);
		r += "2";
	});
	dvr.execute([]() {
		r += "3";
		sch.sleep(100);
		r += "3";
	});

	dvr.step();
	rua::sleep(200);
	dvr.step();

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
