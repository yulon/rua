#include <rua/fiber.hpp>
#include <rua/chan.hpp>

#include <rua/test.hpp>

#include <string>
#include <thread>

namespace {

rua::test _t("fiber", "fiber_driver", []() {
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

rua::test _t3("fiber", "use chan", []() {
	rua::fiber([]() {
		rua::chan<std::string> ch;

		std::thread([ch]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			ch << "ok";
		}).detach();

		RUA_RASSERT(ch.get() == "ok");
	});
});

}
