#ifndef _TMD_FIBER_HPP
#define _TMD_FIBER_HPP

#if defined(_WIN32)
	#include <windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	#ifndef _TMD_UNIX_
		#define _TMD_UNIX_ 1
	#endif

	#include <ucontext.h>
	#include <cstdlib>
	#include <pthread.h>
	#include <unistd.h>
#else
	#error tmd::fiber: not support this OS!
#endif

#include <functional>
#include <memory>
#include <string>
#include <cassert>

namespace tmd {
	#if defined(_TMD_UNIX_)
		static pthread_key_t *_cur_fiber_res_key = []()->pthread_key_t * {
			std::string ptr_str = getenv("____tmd____zwm44qhyah43udr6fr" + std::to_string(getpid()));
			if (ptr_str.empty()) {
				auto ptr = new pthread_key_t;
				pthread_key_create(ptr, nullptr);
				putenv("____tmd____zwm44qhyah43udr6fr" + std::to_string(getpid()), std::to_string(static_cast<unsigned long long>(ptr)));
				return ptr;
			}
			return static_cast<pthread_key_t *>(std::stoull(ptr_str));
		}
	#endif

	class fiber {
		public:
			static void enable_from_this_thread() {
				#if defined(_WIN32)
					if (!GetCurrentFiber()) {
						ConvertThreadToFiber(nullptr);
					}
				#endif
			}

			typedef
			#if defined(_WIN32)
				LPVOID
			#elif defined(_TMD_UNIX_)
				ucontext_t *
			#endif
			native_handle_t;

			fiber(const std::function<void()> &func) : _res(std::make_shared<_res_t>()) {
				_res->func = func;
				_res->owner = this;

				#if defined(_WIN32)
					_win_fiber = CreateFiber(
						0,
						reinterpret_cast<LPFIBER_START_ROUTINE>(&_fiber_func_shell),
						reinterpret_cast<PVOID>(_res.get())
					);
				#elif defined(_TMD_UNIX_)
					getcontext(native_handle());
					native_handle()->uc_stack.ss_flags = 0;
					native_handle()->uc_link = nullptr;
					makecontext(native_handle(), &_fiber_func_shell, reinterpret_cast<void *>(_res.get()));
				#endif
			}

			~fiber() {
				_dtor();
			}

			native_handle_t native_handle() {
				#if defined(_WIN32)
					return _win_fiber;
				#elif defined(_TMD_UNIX_)
					return &_res->uc;
				#endif
			}

			bool joinable() {
				if (_res) {
					if (_res->owner == this) {
						#if defined(_WIN32)
							if (_win_fiber == GetCurrentFiber()) {
								return false;
							}
						#elif defined(_TMD_UNIX_)
							auto cur_fiber_res = reinterpret_cast<_res_t *>(pthread_getspecific(*_cur_fiber_res_key));
							if (_res.get() == cur_fiber_res) {
								return false;
							}
						#endif

						return true;
					}

					_res.reset();

					#if defined(_WIN32)
						if (_win_fiber) {
							_win_fiber = nullptr;
						}
					#endif

					return false;
				}

				#if defined(_WIN32)
					return _win_fiber;
				#elif defined(_TMD_UNIX_)
					return false;
				#endif
			}

			operator bool() {
				return joinable();
			}

			void join() {
				assert(joinable());

				#if defined(_WIN32)
					SwitchToFiber(native_handle());
				#elif defined(_TMD_UNIX_)
					auto cur_fiber_res = reinterpret_cast<_res_t *>(pthread_getspecific(*_cur_fiber_res_key));
					pthread_setspecific(*_cur_fiber_res_key, _res.get());
					if (cur_fiber_res) {
						swapcontext(&cur_fiber_res->uc, native_handle());
					} else {
						setcontext(native_handle());
					}
				#endif
			}

			void detach() {
				if (_res) {
					if (_res->owner == this) {
						_res->owner = nullptr;
					}
					_res.reset();
				}

				#if defined(_WIN32)
					_win_fiber = nullptr;
				#endif
			}

			struct _res_t : std::enable_shared_from_this<_res_t> {
				std::function<void()> func;
				fiber *owner;

				#if defined(_TMD_UNIX_)
					ucontext_t uc;
				#endif
			};

			fiber(native_handle_t ntv_fiber = nullptr) : _res(nullptr)
				#if defined(_WIN32)
					, _win_fiber(ntv_fiber)
				#endif
			{
				#if defined(_TMD_UNIX_)
					if (ntv_fiber) {
						_res->uc = *ntv_fiber;
					}
				#endif
			}

			fiber(native_handle_t ntv_fiber, _res_t *res) : _res(res->shared_from_this())
				#if defined(_WIN32)
					, _win_fiber(ntv_fiber)
				#endif
			{
				res->owner = this;

				#if defined(_TMD_UNIX_)
					if (ntv_fiber) {
						_res->uc = *ntv_fiber;
					}
				#endif
			}

			fiber(const fiber &) = delete;
			fiber& operator=(const fiber &) = delete;

			fiber(fiber &&src) : _res(src._res)
				#if defined(_WIN32)
					, _win_fiber(src._win_fiber)
				#endif
			{
				src._res.reset();
				#if defined(_WIN32)
					src._win_fiber = nullptr;
				#endif
			}

			fiber& operator=(fiber &&src) {
				_dtor();

				_res = src._res;
				#if defined(_WIN32)
					_win_fiber = src._win_fiber;
				#endif

				src._res.reset();
				#if defined(_WIN32)
					src._win_fiber = nullptr;
				#endif

				return *this;
			}

		private:
			std::shared_ptr<_res_t> _res;

			#if defined(_WIN32)
				native_handle_t _win_fiber;
			#endif

			void _dtor() {
				if (_res && _res->owner == this) {
					#if defined(_WIN32)
						if (_res->func) {
							DeleteFiber(_win_fiber);
						}
					#elif defined(_TMD_UNIX_)
						if (_res->func) {
							free(_res->uc.uc_stack.ss_sp);
						}
						if (_res.get() == reinterpret_cast<fiber::_res_t *>(pthread_getspecific(*_cur_fiber_res_key)) && _res.use_count() == 1) {
							pthread_setspecific(*_cur_fiber_res_key, nullptr);
						}
					#endif
					_res->owner = nullptr;
				}
			}

			#if defined(_WIN32)
				static VOID WINAPI _fiber_func_shell(_res_t *res) {
					res->func();
				}
			#elif defined(_TMD_UNIX_)
				static void _fiber_func_shell(_res_t *res) {
					res->func();
				}
			#endif
	};

	namespace this_fiber {
		inline fiber get() {
			#if defined(_WIN32)
				auto cur_win_fiber = GetCurrentFiber();
				assert(cur_win_fiber);
				auto cur_fiber_res = reinterpret_cast<fiber::_res_t *>(GetFiberData());
				if (cur_fiber_res) {
					return fiber(cur_win_fiber, cur_fiber_res);
				}
				return fiber(cur_win_fiber);
			#elif defined(_TMD_UNIX_)
				auto cur_fiber_res = reinterpret_cast<fiber::_res_t *>(pthread_getspecific(*_cur_fiber_res_key));
				if (cur_fiber_res) {
					return fiber(nullptr, cur_fiber_res);
				}
				auto res = std::make_shared<_res_t>();
				pthread_setspecific(*_cur_fiber_res_key, res.get());
				return fiber(nullptr, res.get());
			#endif
		}

		inline fiber get_from_3rd() {
			#if defined(_WIN32)
				auto cur_win_fiber = GetCurrentFiber();
				assert(cur_win_fiber);
				return fiber(cur_win_fiber);
			#elif defined(_TMD_UNIX_)
				auto res = std::make_shared<_res_t>();
				pthread_setspecific(*_cur_fiber_res_key, res.get());
				return fiber(nullptr, res.get());
			#endif
		}

		inline bool revert_to_thread() {
			#if defined(_WIN32)
				if (GetCurrentFiber()) {
					return ConvertFiberToThread();
				}
			#endif

			return true;
		}
	}
}

#endif
