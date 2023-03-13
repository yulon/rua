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
		$data(std::move(init_val)),
		$set(on_set),
		$get(on_get),
		$chg(on_change) {}

	observer(
		T init_val,
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get) :
		$data(std::move(init_val)), $set(on_set), $get(on_get), $chg(nullptr) {}

	observer(
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get,
		const std::function<void(const T &)> &on_change) :
		$data(), $set(on_set), $get(on_get), $chg(on_change) {}

	observer(
		const std::function<T(const T &)> &on_set,
		const std::function<T(const T &)> &on_get) :
		$data(), $set(on_set), $get(on_get), $chg(nullptr) {}

	T get() {
		if ($get) {
			return $get($data);
		}
		return $data;
	}

	operator T() {
		return get();
	}

	void set(const T &value) {
		T cache;

		if ($set) {
			cache = $set(value);
		} else {
			cache = value;
		}

		if ($data != cache) {
			$data = cache;
			if ($chg) {
				$chg($data);
			}
		}
	}

	observer<T> &operator=(const T &value) {
		set(value);
		return *this;
	}

private:
	T $data;
	std::function<T(const T &)> $set;
	std::function<T(const T &)> $get;
	std::function<void(const T &)> $chg;
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
