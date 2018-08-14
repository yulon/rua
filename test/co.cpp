#include <rua/co.hpp>

#include <rua/test.hpp>

#include <iostream>

namespace {

rua::co_pool cp;

rua::test _t("co", "co_pool", []() {
	cp.add([]() {
		std::cout << "sleep 1" << std::endl;
		rua::sleep(300);
		std::cout << "wakeup 1" << std::endl;
	});
	cp.add([]() {
		std::cout << "sleep 2" << std::endl;
		rua::sleep(200);
		std::cout << "wakeup 2" << std::endl;
	});
	cp.add([]() {
		std::cout << "sleep 3" << std::endl;
		rua::sleep(100);
		std::cout << "wakeup 3" << std::endl;
	});
	cp.run();
});

}
