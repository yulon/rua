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

TEST_CASE("reset thread_waker when woke") {
	rua::thread_dozer dzr;
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
	static std::atomic<size_t> recv_c(0);

	rua::thread([]() {
		rua::sleep(100);
		for (size_t i = 0; i < 100; ++i) {
			ch.send(true);
		}
		rua::sleep(100);
		ch.send(false);
	});

	rua::thread([]() {
		rua::sleep(100);
		for (size_t i = 0; i < 100; ++i) {
			ch.send(true);
		}
		rua::sleep(100);
		ch.send(false);
	});

	rua::thread recv_td1([]() {
		while (*ch.recv()) {
			++recv_c;
		}
	});

	rua::thread recv_td2([]() {
		while (*ch.recv()) {
			++recv_c;
		}
	});

	*recv_td1;
	*recv_td2;

	REQUIRE(recv_c.load() == 200);
}
