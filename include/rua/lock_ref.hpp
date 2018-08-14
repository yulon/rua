#ifndef _RUA_LOCK_REF_HPP
#define _RUA_LOCK_REF_HPP

namespace rua {
	class typeless_lock_ref {
		public:
			virtual ~typeless_lock_ref() = default;

			virtual void lock() = 0;

			virtual bool try_lock() = 0;

			virtual void unlock() = 0;

		protected:
			typeless_lock_ref() = default;
	};

	template <typename Lockable>
	class lock_ref : public typeless_lock_ref {
		public:
			lock_ref(Lockable &lck) : _lck(lck) {}

			virtual ~lock_ref() = default;

			virtual void lock() {
				_lck.lock();
			}

			virtual bool try_lock() {
				return _lck.try_lock();
			}

			virtual void unlock() {
				_lck.unlock();
			}

		private:
			Lockable &_lck;
	};

	template <typename Lockable>
	lock_ref<Lockable> make_lock_ref(Lockable &lck) {
		return lock_ref<Lockable>(lck);
	}
}

#endif
