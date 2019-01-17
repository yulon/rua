#include <rua/ucontext.hpp>

#include <rua/test.hpp>

namespace {

rua::test _t("ucontext", "basic", []() {
	static size_t
		main_uc_looping_count = 0,
		sub_uc_1_restore_count = 0,
		sub_uc_1_running_count = 0,
		sub_uc_1_looping_count,
		sub_uc_2_restore_count = 0,
		sub_uc_2_running_count = 0,
		sub_uc_2_looping_count
	;
	static rua::ucontext_t
		main_uc,
		sub_uc_1,
		sub_uc_2
	;
	rua::bin stack(8 * 1024 * 1024);

	rua::clog("get_ucontext(&sub_uc_1)");
	rua::get_ucontext(&sub_uc_1);
	rua::clog("make_ucontext(&sub_uc_1, ...)");
	rua::make_ucontext(&sub_uc_1, [](rua::any_word param) {
		RUA_RASSERT(param.value() == 111);

		++sub_uc_1_running_count;

		sub_uc_1_looping_count = 0;
		for (;;) {
			++sub_uc_1_looping_count;
			rua::clog("set_ucontext(&main_uc)");
			rua::set_ucontext(&main_uc);
		}
	}, 111, stack);

	while (main_uc_looping_count < 3) {
		++main_uc_looping_count;
		++sub_uc_1_restore_count;
		rua::clog("swap_ucontext(&main_uc, &sub_uc_1)");
		rua::swap_ucontext(&main_uc, &sub_uc_1);
		RUA_RASSERT(sub_uc_1_running_count == sub_uc_1_restore_count);
		RUA_RASSERT(sub_uc_1_looping_count == 1);
	}

	RUA_RASSERT(main_uc_looping_count == 3);

	rua::clog("get_ucontext(&sub_uc_2)");
	rua::get_ucontext(&sub_uc_2);
	rua::clog("make_ucontext(&sub_uc_2, ...)");
	rua::make_ucontext(&sub_uc_2, [](rua::any_word param) {
		RUA_RASSERT(param.value() == 222);

		++sub_uc_2_running_count;

		sub_uc_2_looping_count = 0;
		for (;;) {
			++sub_uc_2_looping_count;
			rua::clog("swap_ucontext(&sub_uc_2, &main_uc)");
			rua::swap_ucontext(&sub_uc_2, &main_uc);
		}
	}, 222, stack);

	while (main_uc_looping_count < 6) {
		++main_uc_looping_count;
		++sub_uc_2_restore_count;
		rua::clog("swap_ucontext(&main_uc, &sub_uc_2)");
		rua::swap_ucontext(&main_uc, &sub_uc_2);
		RUA_RASSERT(sub_uc_2_running_count == 1);
		RUA_RASSERT(sub_uc_2_looping_count == sub_uc_2_restore_count);
	}

	RUA_RASSERT(main_uc_looping_count == 6);
});

}
