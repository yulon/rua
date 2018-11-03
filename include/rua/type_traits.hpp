#ifndef _RUA_TYPE_TRAITS_HPP
#define _RUA_TYPE_TRAITS_HPP

#include "macros.hpp"

#include <type_traits>

#include <string>
#ifdef __cpp_lib_string_view
	#include <string_view>
#endif

namespace rua {
	// Compatibility and improvement of std::type_traits.

	#ifdef __cpp_lib_bool_constant
		template <bool Cond>
		using bool_constant = std::bool_constant<Cond>;
	#else
		template <bool Cond>
		using bool_constant = std::integral_constant<bool, Cond>;
	#endif

	template <bool Cond, typename... T>
	struct enable_if;

	template <typename T>
	struct enable_if<true, T> {
		using type = T;
	};

	template <typename T, typename... TArgs>
	struct enable_if<true, T, TArgs...> {
		using type = typename T::template rebind<TArgs...>::type;
	};

	template <typename... T>
	struct enable_if<false, T...> {};

	template <bool Cond, typename T, typename... F>
	struct conditional;

	template <typename T, typename... F>
	struct conditional<true, T, F...> {
		using type = T;
	};

	template <typename T, typename F>
	struct conditional<false, T, F> {
		using type = F;
	};

	template <typename T, typename F, typename... FArgs>
	struct conditional<false, T, F, FArgs...> {
		using type = typename F::template rebind<FArgs...>::type;
	};

	////////////////////////////////////////////////////////////////////////

	template <bool Cond, typename T, typename... Args>
	struct rebind_if;

	template <typename T, typename... Args>
	struct rebind_if<true, T, Args...> {
		using type = typename T::template rebind<Args...>::type;
	};

	template <typename T, typename... Args>
	struct rebind_if<false, T, Args...> {
		using type = T;
	};

	struct default_t {};

	template <typename... Args>
	struct type_switch;

	template <typename Cond, typename Case, typename Result, typename DefaultResult>
	struct type_switch<Cond, Case, Result, DefaultResult> {
		using is_matched = std::is_convertible<Case, Cond>;

		static constexpr bool is_matched_v = is_matched::value;

		using type = typename std::conditional<
			is_matched_v,
			Result,
			DefaultResult
		>::type;

		template <typename... Args>
		struct rebind {
			using type = type_switch<Args...>;
		};
	};

	template <typename Cond, typename Case, typename Result>
	struct type_switch<Cond, Case, Result> :
		type_switch<Cond, Case, Result, default_t>
	{};

	template <typename Top, typename Cond, typename... Others>
	using _type_switch_multicase_handler = rebind_if<!Top::is_matched_v, Top, Cond, Others...>;

	template <typename Cond, typename Case, typename Result, typename... Others>
	struct type_switch<Cond, Case, Result, Others...> :
		_type_switch_multicase_handler<type_switch<Cond, Case, Result>, Cond, Others...>::type
	{};

	template <typename... Args>
	using type_switch_t = typename type_switch<Args...>::type;

	template <typename... Args>
	using switch_true = type_switch<std::true_type, Args...>;

	template <typename... Args>
	using switch_true_t = typename switch_true<Args...>::type;

	template <typename... Args>
	using switch_false = type_switch<std::false_type, Args...>;

	template <typename... Args>
	using switch_false_t = typename switch_false<Args...>::type;

	////////////////////////////////////////////////////////////////////////

	template <typename T>
	struct is_string : bool_constant<
		std::is_same<T, char *>::value
		|| std::is_same<T, const char *>::value
		|| std::is_base_of<std::string, T>::value
		#ifdef __cpp_lib_string_view
			|| std::is_base_of<std::string, T>::value
		#endif
	> {};

	#ifdef __cpp_lib_type_trait_variable_templates
		template <typename T>
		inline constexpr bool is_string_v = is_string<T>::value;
	#endif
}

#endif
