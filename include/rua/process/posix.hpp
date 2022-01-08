#ifndef _RUA_PROCESS_POSIX_HPP
#define _RUA_PROCESS_POSIX_HPP

#include "base.hpp"

#include "../dylib/posix.hpp"
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

	template <
		typename Pid,
		typename = enable_if_t<std::is_integral<Pid>::value>>
	constexpr explicit process(Pid id) : _id(static_cast<pid_t>(id)) {}

	constexpr process(std::nullptr_t = nullptr) : _id(-1) {}

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

	int wait() {
		if (_id <= 0) {
			return -1;
		}
		int status;
		await(waitpid, _id, &status, 0);
		return WIFEXITED(status) ? 0 : WEXITSTATUS(status);
	}

	bool kill() {
		if (_id <= 0) {
			return true;
		}
		if (::kill(_id, SIGKILL) != 0) {
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

	pid_t parent_id() const {
		return getppid();
	}

	process parent() const {
		return process(parent_id());
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

using _process_make_info =
	_baisc_process_make_info<process, file_path, sys_stream>;

namespace _make_process {

class process_maker
	: public process_maker_base<process_maker, _process_make_info> {
public:
	process_maker() = default;

	process_maker(process_maker &&pm) :
		process_maker_base(std::move(static_cast<process_maker_base &&>(pm))) {}

	RUA_OVERLOAD_ASSIGNMENT_R(process_maker)

	~process_maker() {
		if (!_info.file) {
			return;
		}
		start();
	}

	process start() {
		if (!_info.file) {
			return nullptr;
		}

		if (!_info.stderr_w && _info.stdout_w) {
			_info.stderr_w = _info.stdout_w;
		}

		auto id = ::fork();
		if (id) {
			_info.file = "";

			if (id < 0) {
				return nullptr;
			}

			if (_info.stdout_w) {
				_info.stdout_w->detach();
				_info.stdout_w.reset();
			}
			if (_info.stderr_w) {
				_info.stderr_w->detach();
				_info.stderr_w.reset();
			}
			if (_info.stdin_r) {
				_info.stdin_r->detach();
				_info.stdin_r.reset();
			}

			return process(id);
		}

		std::vector<char *> argv;
		argv.emplace_back(&_info.file.str()[0]);
		for (auto &arg : _info.args) {
			argv.emplace_back(&arg[0]);
		}
		argv.emplace_back(nullptr);

		if (_info.stdout_w) {
			out() = std::move(*_info.stdout_w);
		}
		if (_info.stderr_w) {
			err() = std::move(*_info.stderr_w);
		}
		if (_info.stdin_r) {
			in() = std::move(*_info.stdin_r);
		}

		if (_info.work_dir.str().size()) {
			::setenv("PWD", &_info.work_dir.str()[0], 1);
		}

		for (auto &name : _info.dylibs) {
			dylib(name, false);
		}

		process proc;
		if (_info.on_start) {
			proc = this_process();
			_info.on_start(proc);
		}

		::exit(::execvp(_info.file.str().data(), argv.data()));
		return nullptr;
	}

private:
	process_maker(_process_make_info info) :
		process_maker_base(std::move(info)) {}

	friend process_maker make_process(file_path);
};

inline process_maker make_process(file_path file) {
	return process_maker(_process_make_info(std::move(file)));
}

} // namespace _make_process

using namespace _make_process;

}} // namespace rua::posix

#endif
