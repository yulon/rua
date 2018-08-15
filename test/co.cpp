#include <rua/co.hpp>

#include <rua/test.hpp>

namespace {

rua::test _t("co", "co_pool", []() {
	static rua::co_pool cp;
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

	RUA_RASSERT(r == "123321");
});

}
