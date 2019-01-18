#include <rua/fiber.hpp>
#include <rua/channel.hpp>

#include <rua/test.hpp>

#include <string>
#include <thread>

namespace {

rua::test _t("fiber", "fiber_pool", []() {
	static rua::fiber_pool fp;
	static std::string r;

	fp.add([]() {
		r += "1";
		rua::sleep(300);
		r += "1";
	});
	fp.add([]() {
		r += "2";
		rua::sleep(200);
		r += "2";
	});
	fp.add([]() {
		r += "3";
		rua::sleep(100);
		r += "3";
	});
	fp.run();

	RUA_RASSERT(r == "123321");
});

rua::test _t2("fiber", "fiber", []() {
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

	RUA_RASSERT(r == "123321");
});

rua::test _t3("fiber", "use channel", []() {
	rua::fiber([]() {
		rua::channel<std::string> ch;

		std::thread([ch]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			ch << "ok";
		}).detach();

		RUA_RASSERT(ch.get() == "ok");
	});
});

}
