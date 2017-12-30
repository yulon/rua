#ifndef _TMD_OBSERVER_HPP
#define _TMD_OBSERVER_HPP

#include <functional>

namespace rua {
	template <typename T>
	class observer {
		public:
			observer(
				const std::function<T(const T &)> &on_set,
				const std::function<T(const T &)> &on_get,
				const std::function<void(const T &)> &on_change
			) : _set(on_set), _get(on_get), _chg(on_change) {}

			T get() {
				if (_get) {
					return _get(_data);
				}
				return _data;
			}

			operator T() {
				return get();
			}

			void set(const T &value) {
				T cache;

				if (_set) {
					cache = _set(value);
				} else {
					cache = value;
				}

				if (_data != cache) {
					_data = cache;
					if (_chg) {
						_chg(_data);
					}
				}
			}

			const T &operator=(const T &value) {
				set(value);
				return value;
			}

		private:
			T _data;
			std::function<T(const T &)> _set,
			std::function<T(const T &)> _get,
			std::function<void(const T &)> _chg
	};
}

#endif
