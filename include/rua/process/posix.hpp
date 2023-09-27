#ifndef _rua_process_posix_hpp
#define _rua_process_posix_hpp

#include "base.hpp"

#include "../dylib/posix.hpp"
#include "../file/posix.hpp"
#include "../stdio/posix.hpp"
#include "../string.hpp"
#include "../conc/await.hpp"
#include "../conc/future.hpp"
#include "../conc/then.hpp"
#include "../conc/wait.hpp"
#include "../util.hpp"

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

class process : private enable_await_operators {
public:
	using native_handle_t = pid_t;

	////////////////////////////////////////////////////////////////

	template <
		typename Pid,
		typename = enable_if_t<std::is_integral<Pid>::value>>
	constexpr explicit process(Pid id) : $id(static_cast<pid_t>(id)) {}

	constexpr process(std::nullptr_t = nullptr) : $id(-1) {}

	~process() {
		reset();
	}

	process(process &&src) : $id(src.$id) {
		if (src) {
			src.$id = -1;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(process)

	pid_t id() const {
		return $id;
	}

	bool operator==(const process &target) const {
		return id() == target.id();
	}

	bool operator!=(const process &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		return $id;
	}

	operator bool() const {
		return $id >= 0;
	}

	future<int> RUA_OPERATOR_AWAIT() const {
		if (!$id) {
			return 0;
		}
		auto id = $id;
		return parallel([id]() -> any_word {
			int status;
			waitpid(id, &status, 0);
			return WIFEXITED(status) ? 0 : WEXITSTATUS(status);
		});
	}

	void kill() {
		if ($id <= 0) {
			return;
		}
		::kill($id, SIGKILL);
		$id = -1;
	}

	bool suspend() {
		return ::kill($id, SIGSTOP) == 0;
	}

	bool resume() {
		return ::kill($id, SIGCONT) == 0;
	}

	file_path path() const {
		if ($id < 0) {
			return "";
		}
		char buf[PATH_MAX];
		auto n = readlink(
			("/proc/" + std::to_string($id) + "/exe").c_str(),
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
		if ($id < 0) {
			return;
		}
		$id = -1;
	}

	void detach() {
		reset();
	}

private:
	::pid_t $id;
};

namespace _this_process {

inline process this_process() {
	return process(this_pid());
}

inline file_path working_dir() {
	auto c_str = get_current_dir_name();
	file_path r(c_str);
	::free(c_str);
	return r;
}

inline bool work_at(const file_path &path) {
#ifndef NDEBUG
	auto r =
#else
	return
#endif
		!::chdir(path.str().c_str());
#ifndef NDEBUG
	assert(r);
	return r;
#endif
}

inline std::string get_env(string_view name) {
	return ::getenv(std::string(name).c_str());
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

	RUA_OVERLOAD_ASSIGNMENT(process_maker)

	~process_maker() {
		if (!$info.file) {
			return;
		}
		start();
	}

	process start() {
		if (!$info.file) {
			return nullptr;
		}

		if (!$info.stderr_w && $info.stdout_w) {
			$info.stderr_w = $info.stdout_w;
		}

		auto id = ::fork();
		if (id) {
			$info.file = "";

			if (id < 0) {
				return nullptr;
			}

			if ($info.stdout_w) {
				$info.stdout_w->detach();
				$info.stdout_w.reset();
			}
			if ($info.stderr_w) {
				$info.stderr_w->detach();
				$info.stderr_w.reset();
			}
			if ($info.stdin_r) {
				$info.stdin_r->detach();
				$info.stdin_r.reset();
			}

			return process(id);
		}

		std::vector<char *> argv;
		argv.emplace_back(&$info.file.str()[0]);
		for (auto &arg : $info.args) {
			argv.emplace_back(&arg[0]);
		}
		argv.emplace_back(nullptr);

		if ($info.stdout_w) {
			out() = std::move(*$info.stdout_w);
		}
		if ($info.stderr_w) {
			err() = std::move(*$info.stderr_w);
		}
		if ($info.stdin_r) {
			in() = std::move(*$info.stdin_r);
		}

		if ($info.work_dir.str().size()) {
			::setenv("PWD", $info.work_dir.str().c_str(), 1);
		}

		for (auto &env : $info.envs) {
			::setenv(env.first.c_str(), env.second.c_str(), 1);
		}

		for (auto &name : $info.dylibs) {
			dylib(name, false);
		}

		process proc;
		if ($info.on_start) {
			proc = this_process();
			$info.on_start(proc);
		}

		::exit(::execvp($info.file.str().data(), argv.data()));
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
