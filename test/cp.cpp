#include <rua/cp.hpp>

#include <gtest/gtest.h>

#include <iostream>

rua::cp::coro_pool copo;

TEST(Cp, CoroPool) {
	copo.bind_this_thread();
	copo.go([]() {
		std::cout << "sleep 1" << std::endl;
		copo.sleep(100);
		std::cout << "wakeup 1" << std::endl;
	});
	copo.go([]() {
		std::cout << "sleep 2" << std::endl;
		copo.sleep(200);
		std::cout << "wakeup 2" << std::endl;
	});
	copo.go([]() {
		std::cout << "sleep 3" << std::endl;
		copo.sleep(300);
		std::cout << "wakeup 3" << std::endl;
	});
	copo.handle();
}
