#ifndef _RUA_PROCESS_BASE_HPP
#define _RUA_PROCESS_BASE_HPP

#include "../any_word.hpp"
#include "../optional.hpp"
#include "../range.hpp"
#include "../string/conv.hpp"
#include "../types/traits.hpp"

#include <functional>
#include <list>

namespace rua {

template <typename Process, typename FilePath, typename StdioStream>
class _baisc_process_make_info {
public:
	using process_t = Process;
	using file_path_t = FilePath;
	using stdio_stream_t = StdioStream;

	_baisc_process_make_info() = default;

	_baisc_process_make_info(file_path_t file) :
		_file(std::move(file)), _hide(false) {}

protected:
	FilePath _file, _work_dir;
	std::list<std::string> _args;
	optional<StdioStream> _stdout_w, _stderr_w, _stdin_r;
	bool _hide;
	std::function<void(process_t &)> _on_start;
};

template <typename ProcessMaker, typename ProcessMakeInfo>
class process_maker_base : public ProcessMakeInfo {
public:
	using process_t = typename ProcessMakeInfo::process_t;
	using file_path_t = typename ProcessMakeInfo::file_path_t;
	using stdio_stream_t = typename ProcessMakeInfo::stdio_stream_t;

	process_maker_base() = default;

	template <typename Arg>
	ProcessMaker &arg(Arg &&a) {
		this->_args.emplace_back(to_string(std::forward<Arg>(a)));
		return *_this();
	}

	ProcessMaker &args(std::list<std::string> a_str_li) {
		if (!a_str_li.size()) {
			return *_this();
		}
		if (this->_args.size()) {
			this->_args.splice(this->_args.end(), std::move(a_str_li));
			return *_this();
		}
		this->_args = std::move(a_str_li);
		return *_this();
	}

	template <RUA_STRING_RANGE(StrList)>
	enable_if_t<
		!std::is_base_of<std::list<std::string>, decay_t<StrList>>::value,
		ProcessMaker &>
	args(StrList &&a_str_li) {
		std::list<std::string> backs;
		RUA_RANGE_FOR(auto &a_str, a_str_li, {
			backs.emplace_back(to_string(std::move(a_str)));
		})
		return args(std::move(backs));
	}

	ProcessMaker &args() {
		return *_this();
	}

	template <typename... Args>
	enable_if_t<!is_range<front_t<Args &&...>>::value, ProcessMaker &>
	args(Args &&...a) {
		return arg(std::forward<Args>(a)...);
	}

	template <typename... Args>
	enable_if_t<(sizeof...(Args) > 1), ProcessMaker &> args(Args &&...a) {
		return args(
			std::list<std::string>({to_string(std::forward<Args>(a))...}));
	}

	ProcessMaker &work_at(file_path_t work_dir) {
		this->_work_dir = std::move(work_dir);
		return *_this();
	}

	ProcessMaker &hide() {
		this->_hide = true;
		return *_this();
	}

	ProcessMaker &stdout_to(stdio_stream_t stdout_w) {
		this->_stdout_w = std::move(stdout_w);
		return *_this();
	}

	ProcessMaker &stderr_to(stdio_stream_t stderr_w) {
		this->_stderr_w = std::move(stderr_w);
		return *_this();
	}

	ProcessMaker &stdin_from(stdio_stream_t stdin_r) {
		this->_stdin_r = std::move(stdin_r);
		return *_this();
	}

	void on_start(std::function<void(process_t &)> cb) {
		this->_on_start = std::move(cb);
	}

	any_word run() {
		return _this()->start().wait_for_exit();
	}

protected:
	process_maker_base(ProcessMakeInfo info) :
		ProcessMakeInfo(std::move(info)) {}

private:
	ProcessMaker *_this() {
		return static_cast<ProcessMaker *>(this);
	}
};

} // namespace rua

#endif
