#include <rua/thread.hpp>
#include <rua/time.hpp>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("thread") {
	static std::string r;

	rua::thread([]() mutable {
		rua::sleep(100);
		r = "ok";
	}).wait();

	REQUIRE(r == "ok");
}

TEST_CASE("reset thread_waker when woke") {
	auto dzr = rua::this_dozer();
	auto wkr = dzr->get_waker();
	for (size_t i = 0; i < 3; ++i) {
		auto t = rua::tick();
		rua::thread([=]() mutable {
			rua::sleep(200);
			wkr->wake();
		});
		REQUIRE(dzr->doze(300));
		REQUIRE(rua::tick() - t > 100);
	}
}

TEST_CASE("use chan on thread") {
	static rua::chan<std::string> ch;

	rua::thread([]() mutable {
		rua::sleep(100);
		ch << "ok";
	});

	REQUIRE(ch.pop() == "ok");
}
