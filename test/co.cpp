#include <rua/co.hpp>

#include <gtest/gtest.h>

#include <iostream>

rua::co_pool cp;

TEST(co, co_pool) {
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
}
