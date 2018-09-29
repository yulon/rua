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
	RUA_RASSERT(b->x == a->x);
	RUA_RASSERT(b->y == a->y);

	rua::itf<foo> c(b);
	RUA_RASSERT(c->x == a->x);
	RUA_RASSERT(c->y == a->y);

	rua::itf<foo> d(b);
	RUA_RASSERT(d->x == a->x);
	RUA_RASSERT(d->y == a->y);

	a->x = 222;
	a->y = 333;

	RUA_RASSERT(b->x == a->x);
	RUA_RASSERT(b->y == a->y);

	RUA_RASSERT(c->x == a->x);
	RUA_RASSERT(c->y == a->y);

	RUA_RASSERT(d->x == a->x);
	RUA_RASSERT(d->y == a->y);
});

}
