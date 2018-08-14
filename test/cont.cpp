#include <rua/cont.hpp>

#include <gtest/gtest.h>

#include <iostream>

size_t
	main_ct_looping_count,
	sub_ct_1_restore_count,
	sub_ct_1_running_count,
	sub_ct_1_looping_count,
	sub_ct_2_restore_count,
	sub_ct_2_running_count,
	sub_ct_2_looping_count
;

static rua::cont
	main_ct,
	sub_ct_1,
	sub_ct_2
;

TEST(cont, restore) {
	rua::bin stack(8 * 1024 * 1024);

	main_ct_looping_count = 0;

	sub_ct_1_running_count = 0;

	sub_ct_1.make([](rua::any_word param) {
		ASSERT_EQ(param.value(), 111);

		++sub_ct_1_running_count;

		sub_ct_1_looping_count = 0;
		for (;;) {
			++sub_ct_1_looping_count;
			main_ct.restore();
		}
	}, 111, stack);

	sub_ct_1_restore_count = 0;

	while (main_ct_looping_count < 3) {
		++main_ct_looping_count;
		++sub_ct_1_restore_count;
		sub_ct_1.restore(main_ct);
		ASSERT_EQ(sub_ct_1_running_count, sub_ct_1_restore_count);
		ASSERT_EQ(sub_ct_1_looping_count, 1);
	}

	ASSERT_EQ(main_ct_looping_count, 3);

	sub_ct_2_running_count = 0;

	sub_ct_2.make([](rua::any_word param) {
		ASSERT_EQ(param.value(), 222);

		++sub_ct_2_running_count;

		sub_ct_2_looping_count = 0;
		for (;;) {
			++sub_ct_2_looping_count;
			main_ct.restore(sub_ct_2);
		}
	}, 222, stack);

	sub_ct_2_restore_count = 0;

	while (main_ct_looping_count < 6) {
		++main_ct_looping_count;
		++sub_ct_2_restore_count;
		sub_ct_2.restore(main_ct);
		ASSERT_EQ(sub_ct_2_running_count, 1);
		ASSERT_EQ(sub_ct_2_looping_count, sub_ct_2_restore_count);
	}

	ASSERT_EQ(main_ct_looping_count, 6);
}
