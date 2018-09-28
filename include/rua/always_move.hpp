#ifndef _RUA_ALWAYS_MOVE_HPP
#define _RUA_ALWAYS_MOVE_HPP

namespace rua {
	template <typename T>
	class always_move {
		public:
			T value;

			always_move() = default;

			always_move(always_move<T> &&src) : value(std::move(src.value)) {}

			always_move<T> &operator=(always_move<T> &&src) {
				value = std::move(src.value);
			}

			always_move(const always_move<T> &src) : value(std::move(src.value)) {}

			always_move<T> &operator=(const always_move<T> &src) {
				value = std::move(src.value);
			}

			always_move(T &&src_value) : value(std::move(src_value)) {}

			always_move<T> &operator=(T &&src_value) {
				value = std::move(src_value);
			}

			always_move(const T &src_value) : value(std::move(src_value)) {}

			always_move<T> &operator=(const T &src_value) {
				value = std::move(src_value);
			}

			operator T &&() && {
				return std::move(value);
			}

			operator T &() & {
				return value;
			}

			operator const T &() const & {
				return value;
			}

			T *operator->() {
				return &value;
			}

			const T *operator->() const {
				return &value;
			}
	};
}

#endif
