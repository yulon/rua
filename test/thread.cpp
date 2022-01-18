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

	auto t = rua::tick();
	rua::thread([=]() mutable {
		rua::sleep(200);
		wkr->wake();
	});
	CHECK(dzr->doze());
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() mutable {
		rua::sleep(200);
		wkr->wake();
	});
	CHECK(dzr->doze());
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() mutable {
		rua::sleep(200);
		wkr->wake();
	});
	CHECK(dzr->doze(1000));
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() mutable {
		rua::sleep(200);
		wkr->wake();
	});
	CHECK(dzr->doze(300));
	CHECK((rua::tick() - t).milliseconds() > 100);
}

TEST_CASE("use chan on thread") {
	static rua::chan<std::string> ch;

	rua::thread([]() mutable {
		rua::sleep(100);
		ch << "ok";
	});

	REQUIRE(ch.pop() == "ok");
}
