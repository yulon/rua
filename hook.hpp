#ifndef _TMD_HOOK_HPP
#define _TMD_HOOK_HPP

#include <MinHook.h>

#include <functional>
#include <vector>
#include <atomic>

namespace tmd {
	extern std::atomic<size_t> _hook_count;

	template <typename T>
	class hook {
		public:
			using target_type = T;

			constexpr hook() : _t(nullptr), _o(nullptr) {}

			hook(T target, T detour) : _t(target) {
				if (++_hook_count == 1) {
					MH_Initialize();
				}
				MH_CreateHook(reinterpret_cast<LPVOID>(target), reinterpret_cast<LPVOID>(detour), reinterpret_cast<LPVOID *>(&_o));
				MH_EnableHook(reinterpret_cast<LPVOID>(target));
			}

			hook(const hook<T> &) = delete;

			hook<T> &operator=(const hook<T> &) = delete;

			hook(hook<T> &&src) : _t(src._t), _o(src._o) {
				if (src._t) {
					src._t = nullptr;
				}
			}

			hook<T> &operator=(hook<T> &&src) {
				unhook();

				if (src._t) {
					_t = src._t;
					_o = src._o;

					src._t = nullptr;
				}

				return *this;
			}

			~hook() {
				unhook();
			}

			template <typename... A>
			auto orig_fn(A&&... a) const -> decltype((*reinterpret_cast<T>(0))(std::forward<A>(a)...)) {
				return _o(std::forward<A>(a)...);
			}

			void unhook() {
				if (_t) {
					MH_DisableHook(reinterpret_cast<LPVOID>(_t));
					MH_RemoveHook(reinterpret_cast<LPVOID>(_t));
					_t = nullptr;
					if (!--_hook_count) {
						MH_Uninitialize();
					}
				}
			}

			operator bool() const {
				return _t;
			}

			template <typename... A>
			void assign(A&&... a) {
				*this = hook<T>(std::forward<A>(a)...);
			}

		private:
			T _t, _o;
	};
}

#endif
