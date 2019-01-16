#include <rua/coroutine.hpp>
#include <rua/channel.hpp>

#include <rua/test.hpp>

#include <string>
#include <thread>

namespace {

rua::test _t("coroutine", "coroutine_pool", []() {
	static rua::coroutine_pool cp;
	static std::string r;

	cp.add([]() {
		r += "1";
		rua::sleep(300);
		r += "1";
	});
	cp.add([]() {
		r += "2";
		rua::sleep(200);
		r += "2";
	});
	cp.add([]() {
		r += "3";
		rua::sleep(100);
		r += "3";
	});
	cp.run();

	RUA_PANIC(r == "123321");
});

rua::test _t2("coroutine", "co", []() {
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

	RUA_PANIC(r == "123321");
});

rua::test _t3("coroutine", "use channel", []() {
	rua::co([]() {
		rua::channel<std::string> ch;

		std::thread([ch]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			ch << "ok";
		}).detach();

		RUA_PANIC(ch.get() == "ok");
	});
});

}
