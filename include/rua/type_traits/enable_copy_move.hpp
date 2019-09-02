#ifndef _RUA_TYPE_TRAITS_ENABLE_COPY_MOVE_HPP
#define _RUA_TYPE_TRAITS_ENABLE_COPY_MOVE_HPP

#include "../macros.hpp"

namespace rua {

template <bool IsCopyable, bool IsMovable>
class enable_copy_move;

template <>
class enable_copy_move<true, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;
	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
};

template <>
class enable_copy_move<true, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;

	RUA_OVERLOAD_ASSIGNMENT_L(enable_copy_move)
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT_R(enable_copy_move)
};

template <>
class enable_copy_move<false, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = delete;
	enable_copy_move(enable_copy_move &&) = delete;

	enable_copy_move &operator=(const enable_copy_move &) = delete;
	enable_copy_move &operator=(enable_copy_move &&) = delete;
};

template <typename T>
using enable_copy_move_like = enable_copy_move<
	std::is_copy_constructible<T>::value,
	std::is_move_constructible<T>::value>;

} // namespace rua

#endif
