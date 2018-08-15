#include <rua/cont.hpp>

#include <rua/test.hpp>

namespace {

rua::test _t("cont", "restore", []() {
	static size_t
		main_ct_looping_count = 0,
		sub_ct_1_restore_count = 0,
		sub_ct_1_running_count = 0,
		sub_ct_1_looping_count,
		sub_ct_2_restore_count = 0,
		sub_ct_2_running_count = 0,
		sub_ct_2_looping_count
	;
	static rua::cont
		main_ct,
		sub_ct_1,
		sub_ct_2
	;
	rua::bin stack(8 * 1024 * 1024);

	sub_ct_1.make([](rua::any_word param) {
		RUA_RASSERT(param.value() == 111);

		++sub_ct_1_running_count;

		sub_ct_1_looping_count = 0;
		for (;;) {
			++sub_ct_1_looping_count;
			main_ct.restore();
		}
	}, 111, stack);

	while (main_ct_looping_count < 3) {
		++main_ct_looping_count;
		++sub_ct_1_restore_count;
		sub_ct_1.restore(main_ct);
		RUA_RASSERT(sub_ct_1_running_count == sub_ct_1_restore_count);
		RUA_RASSERT(sub_ct_1_looping_count == 1);
	}

	RUA_RASSERT(main_ct_looping_count == 3);

	sub_ct_2.make([](rua::any_word param) {
		RUA_RASSERT(param.value() == 222);

		++sub_ct_2_running_count;

		sub_ct_2_looping_count = 0;
		for (;;) {
			++sub_ct_2_looping_count;
			main_ct.restore(sub_ct_2);
		}
	}, 222, stack);

	while (main_ct_looping_count < 6) {
		++main_ct_looping_count;
		++sub_ct_2_restore_count;
		sub_ct_2.restore(main_ct);
		RUA_RASSERT(sub_ct_2_running_count == 1);
		RUA_RASSERT(sub_ct_2_looping_count == sub_ct_2_restore_count);
	}

	RUA_RASSERT(main_ct_looping_count == 6);
});

}
