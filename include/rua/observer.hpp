#ifndef _rua_observer_hpp
#define _rua_observer_hpp

#include <functional>

namespace rua {
template <typename T>
class observer {
public:
	observer(
		T init_val,
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get,
		const std::function<void(const T &)> &on_change) :
		_data(std::move(init_val)),
		_set(on_set),
		_get(on_get),
		_chg(on_change) {}

	observer(
		T init_val,
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get) :
		_data(std::move(init_val)), _set(on_set), _get(on_get), _chg(nullptr) {}

	observer(
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get,
		const std::function<void(const T &)> &on_change) :
		_data(), _set(on_set), _get(on_get), _chg(on_change) {}

	observer(
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get) :
		_data(), _set(on_set), _get(on_get), _chg(nullptr) {}

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

	observer<T> &operator=(const T &value) {
		set(value);
		return *this;
	}

private:
	T _data;
	std::function<T(const T &)> _set;
	std::function<T(const T &)> _get;
	std::function<void(const T &)> _chg;
};

template <typename R, typename V>
inline bool set_and_is_changed(R &receiver, V &&value) {
	if (receiver == std::forward<V>(value)) {
		receiver = std::forward<V>(value);
		return false;
	}
	receiver = std::forward<V>(value);
	return true;
}

template <typename R, typename V>
inline bool set_when_changing(R &receiver, V &&value) {
	if (receiver == std::forward<V>(value)) {
		return false;
	}
	receiver = std::forward<V>(value);
	return true;
}
} // namespace rua

#endif
