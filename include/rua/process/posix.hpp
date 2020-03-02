#ifndef _RUA_PROCESS_POSIX_HPP
#define _RUA_PROCESS_POSIX_HPP

#include "../sched/block_call.hpp"
#include "../stdio/posix.hpp"
#include "../string.hpp"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstddef>
#include <cstdlib>
#include <vector>

namespace rua { namespace posix {

using pid_t = ::pid_t;

namespace _this_pid {

RUA_FORCE_INLINE pid_t this_pid() {
	return getpid();
}

} // namespace _this_pid

using namespace _this_pid;

class process {
public:
	using native_handle_t = pid_t;

	////////////////////////////////////////////////////////////////

	static process find(string_view name) {
		return nullptr;
	}

	static process wait_for_found(string_view name, ms interval = 100) {
		process p;
		for (;;) {
			p = find(name);
			if (p) {
				break;
			}
			rua::sleep(interval);
		}
		return p;
	}

	////////////////////////////////////////////////////////////////

	constexpr process() : _id(-1) {}

	explicit process(
		std::string file,
		std::vector<std::string> args = {},
		std::string pwd = "",
		bool freeze_at_startup = false,
		sys_stream stdout_w = out(),
		sys_stream stderr_w = err(),
		sys_stream stdin_r = in()) {

		_id = ::fork();
		if (_id) {
			return;
		}
		_id = -1;

		std::vector<char *> argv(2 + args.size());
		argv[0] = &file[0];
		size_t i;
		for (i = 0; i < args.size(); ++i) {
			argv[i + 1] = &args[i][0];
		}
		argv[i + 1] = nullptr;

		set_out(stdout_w);
		set_err(stderr_w);
		set_in(stdin_r);

		if (pwd.empty()) {
			::exit(::execvp(file.data(), argv.data()));
		}
		pwd = "PWD=" + pwd;
		char *envp[2]{&pwd[0], nullptr};
		::exit(::execvpe(file.data(), argv.data(), &envp[0]));
	}

	constexpr explicit process(pid_t id) : _id(id) {}

	template <
		typename NullPtr,
		typename = enable_if_t<std::is_same<NullPtr, std::nullptr_t>::value>>
	constexpr process(NullPtr) : process() {}

	~process() {
		reset();
	}

	process(process &&src) : _id(src._id) {
		if (src) {
			src._id = -1;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(process)

	pid_t id() const {
		return _id;
	}

	bool operator==(const process &target) const {
		return id() == target.id();
	}

	bool operator!=(const process &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		return _id;
	}

	operator bool() const {
		return _id >= 0;
	}

	void unfreeze() {}

	int wait_for_exit() {
		if (_id <= 0) {
			return -1;
		}
		unfreeze();
		int status;
		block_call(waitpid, _id, &status, 0);
		return WIFEXITED(status) ? 0 : WEXITSTATUS(status);
	}

	void kill() {
		if (_id <= 0) {
			return;
		}
		::kill(_id, 0);
		reset();
	}

	std::string file_path() {
		return "";
	}

	void reset() {
		if (_id < 0) {
			return;
		}
		_id = -1;
	}

private:
	::pid_t _id;
};

namespace _this_process {

RUA_FORCE_INLINE process this_process() {
	return process(this_pid());
}

} // namespace _this_process

using namespace _this_process;

}} // namespace rua::posix

#endif
