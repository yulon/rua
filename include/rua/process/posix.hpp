#ifndef _RUA_PROCESS_POSIX_HPP
#define _RUA_PROCESS_POSIX_HPP

#include "../file/posix.hpp"
#include "../sched/await.hpp"
#include "../stdio/posix.hpp"
#include "../string.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

namespace rua { namespace posix {

using pid_t = ::pid_t;

namespace _this_pid {

inline pid_t this_pid() {
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

	static process wait_for_found(string_view name, duration interval = 100) {
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
		file_path file,
		std::vector<std::string> args = {},
		file_path pwd = "",
		bool lazy_start = false,
		sys_stream stdout_w = out(),
		sys_stream stderr_w = err(),
		sys_stream stdin_r = in()) {

		_id = ::fork();
		if (_id) {
			return;
		}
		_id = -1;

		std::vector<char *> argv(2 + args.size());
		argv[0] = &file.str()[0];
		size_t i;
		for (i = 0; i < args.size(); ++i) {
			argv[i + 1] = &args[i][0];
		}
		argv[i + 1] = nullptr;

		out() = std::move(stdout_w);
		err() = std::move(stderr_w);
		in() = std::move(stdin_r);

		if (pwd.str().size()) {
			::setenv("PWD", &pwd.str()[0], 1);
		}
		::exit(::execvp(file.str().data(), argv.data()));
	}

	constexpr explicit process(pid_t id) : _id(id) {}

	template <
		typename NullPtr,
		typename = enable_if_t<is_null_pointer<NullPtr>::value>>
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

	void start() {}

	int wait_for_exit() {
		if (_id <= 0) {
			return -1;
		}
		start();
		int status;
		await(waitpid, _id, &status, 0);
		return WIFEXITED(status) ? 0 : WEXITSTATUS(status);
	}

	bool kill() {
		if (_id <= 0) {
			return true;
		}
		if (::kill(_id, 0) != 0) {
			return false;
		}
		_id = -1;
		return true;
	}

	bool suspend() {
		return ::kill(_id, SIGSTOP) == 0;
	}

	bool resume() {
		return ::kill(_id, SIGCONT) == 0;
	}

	file_path path() const {
		if (_id < 0) {
			return "";
		}
		char buf[PATH_MAX];
		auto n = readlink(
			("/proc/" + std::to_string(_id) + "/exe").c_str(),
			&buf[0],
			PATH_MAX);
		if (n < 0) {
			return "";
		}
		return std::string(&buf[0], n);
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

inline process this_process() {
	return process(this_pid());
}

} // namespace _this_process

using namespace _this_process;

}} // namespace rua::posix

#endif
