#include <rua/poly.hpp>

#include <rua/test.hpp>

namespace {

rua::test _t("poly", "obj", []() {
	struct foo {
		int x, y;
	};

	rua::obj<foo> a, b(foo{1, 2});
});

rua::test _t2("poly", "itf", []() {
	struct foo {
		int x, y;
	};

	struct bar : foo {
		bar() = default;
	};

	rua::obj<bar> a;
	a->x = 123;
	a->y = 321;

	rua::obj<bar> b(a);
	RUA_PANIC(b->x == a->x);
	RUA_PANIC(b->y == a->y);

	rua::itf<foo> c(b);
	RUA_PANIC(c->x == a->x);
	RUA_PANIC(c->y == a->y);

	rua::itf<foo> d(b);
	RUA_PANIC(d->x == a->x);
	RUA_PANIC(d->y == a->y);

	a->x = 222;
	a->y = 333;

	RUA_PANIC(b->x == a->x);
	RUA_PANIC(b->y == a->y);

	RUA_PANIC(c->x == a->x);
	RUA_PANIC(c->y == a->y);

	RUA_PANIC(d->x == a->x);
	RUA_PANIC(d->y == a->y);
});

}
