namespace rua {
	template <typename T>
	class rval {
		public:
			T value;

			rval() = default;

			rval(rval<T> &&src) : value(std::move(src.value)) {}

			rval<T> &operator=(rval<T> &&src) {
				value = std::move(src.value);
			}

			rval(const rval<T> &src) : value(std::move(src.value)) {}

			rval<T> &operator=(const rval<T> &src) {
				value = std::move(src.value);
			}

			rval(T &&src_value) : value(std::move(src_value)) {}

			rval<T> &operator=(T &&src_value) {
				value = std::move(src_value);
			}

			rval(const T &src_value) : value(std::move(src_value)) {}

			rval<T> &operator=(const T &src_value) {
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
