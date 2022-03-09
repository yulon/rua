#include <rua/thread.hpp>
#include <rua/time.hpp>

#include <doctest/doctest.h>

#include <atomic>
#include <string>

TEST_CASE("thread") {
	static std::string r;

	rua::thread([]() {
		rua::sleep(100);
		r = "ok";
	}).wait();

	REQUIRE(r == "ok");
}

TEST_CASE("reset waker when woke") {
	rua::dozer dzr;
	auto wkr = dzr.get_waker();

	auto t = rua::tick();
	rua::thread([=]() {
		rua::sleep(200);
		wkr.lock()->wake();
	});
	CHECK(dzr.doze());
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() {
		rua::sleep(200);
		wkr.lock()->wake();
	});
	CHECK(dzr.doze());
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() {
		rua::sleep(200);
		wkr.lock()->wake();
	});
	CHECK(dzr.doze(1000));
	CHECK((rua::tick() - t).milliseconds() > 100);

	t = rua::tick();
	rua::thread([=]() {
		rua::sleep(200);
		wkr.lock()->wake();
	});
	CHECK(dzr.doze(300));
	CHECK((rua::tick() - t).milliseconds() > 100);
}

TEST_CASE("use chan on thread") {
	static rua::chan<std::string> ch;

	rua::thread([]() {
		rua::sleep(100);
		ch.send("ok");
	});

	REQUIRE(*ch.recv() == "ok");
}

TEST_CASE("use chan on multi-thread") {
	static rua::chan<bool> ch;

	rua::thread([]() {
		rua::sleep(100);
		for (size_t i = 0; i < 100; ++i) {
			ch.send(true);
		}
	});

	rua::thread([]() {
		rua::sleep(100);
		for (size_t i = 0; i < 100; ++i) {
			ch.send(true);
		}
	});

	static rua::chan<bool> ch2;

	rua::thread([]() {
		while (*ch.recv()) {
			ch2.send(true);
		}
	});

	rua::thread([]() {
		while (*ch.recv()) {
			ch2.send(false);
		}
	});

	size_t i = 0;
	for (; i < 200; ++i) {
		*ch2.recv();
	}
	REQUIRE(i == 200);
}
