#include <rua/chan.hpp>

#include <rua/test.hpp>

#include <string>
#include <thread>

namespace {

rua::test _t("chan", "basic", []() {
	rua::chan<std::string> ch;

	std::thread([ch]() mutable {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ch << "ok";
	}).detach();

	RUA_RASSERT(ch.get() == "ok");
});

}
