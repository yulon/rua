#include <rua/ref.hpp>

#include <doctest/doctest.h>

#include <memory>

TEST_CASE("use ref for interface") {
	struct animal {
		virtual ~animal() = default;
		virtual size_t age() const = 0;
	};

	using animal_i = rua::ref<animal>;

	struct dog : animal {
		virtual ~dog() = default;
		virtual size_t age() const {
			return 123;
		}
		size_t color() const {
			return 321;
		}
	};

	dog dog_lval;
	std::unique_ptr<dog> dog_unique_ptr_lval(new dog);

	////////////////////////////////////////////////////////////////////////////

	// save raw pointer
	animal_i aml1(dog_lval);			// Impl &
	animal_i aml2(&dog_lval);			// Impl *
	animal_i aml3(dog_unique_ptr_lval); // std::unique_ptr<Impl> &

	// save shared pointer
	animal_i aml4(std::make_shared<dog>());		  // std::shared_ptr<Impl>
	animal_i aml5(dog{});						  // Impl &&
	animal_i aml6(std::unique_ptr<dog>(new dog)); // std::unique_ptr<Impl> &&

	// save pointer type same as source jeko
	animal_i aml7(aml6); // jeko

	// null
	animal_i aml8;
	animal_i aml9(nullptr);

	////////////////////////////////////////////////////////////////////////////

	REQUIRE(aml1->age() == dog_lval.age());
	REQUIRE(aml1.to<dog>()->age() == dog_lval.age());
	REQUIRE(aml2->age() == dog_lval.age());
	REQUIRE(aml3->age() == dog_lval.age());
	REQUIRE(aml4->age() == dog_lval.age());
	REQUIRE(aml5->age() == dog_lval.age());
	REQUIRE(aml6->age() == dog_lval.age());
	REQUIRE(aml7->age() == dog_lval.age());
	REQUIRE(!aml8);
	REQUIRE(!aml9);
}
