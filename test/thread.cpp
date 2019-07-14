#include <rua/chan.hpp>
#include <rua/thread.hpp>

#include <rua/test.hpp>

#include <string>

namespace {

rua::test _t("thread", "thread", []() {
	static std::string r;

	rua::thread([]() mutable {
		rua::sleep(100);
		r = "ok";
	}).wait_for_exit();

	RUA_RASSERT(r == "ok");
});

rua::test _t2("thread", "use chan", []() {
	rua::chan<std::string> ch;

	rua::thread([ch]() mutable {
		rua::sleep(100);
		ch << "ok";
	});

	RUA_RASSERT(ch.get() == "ok");
});

}
