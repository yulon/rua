#ifndef _RUA_TEST_HPP
#define _RUA_TEST_HPP

#include "macros.hpp"
#include "rassert.hpp"

#include <map>
#include <list>
#include <functional>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>

namespace rua {
	class test {
		public:
			test(std::string group_name, std::string case_name, std::function<void()> proc) {
				_map()[group_name].push_back(_info_t{
					std::move(case_name),
					std::move(proc)
				});
			}

			static void run() {
				std::cout << std::endl;

				auto &map = _map();
				for (auto &pr : map) {
					auto &gn = pr.first;
					auto &li = pr.second;

					std::cout << _bar(gn, '=', 4) << std::endl;
					auto tp = std::chrono::steady_clock::now();

					for (auto &info : li) {
						std::cout << _bar(info.case_name, '-', 2) << std::endl;
						auto tp2 = std::chrono::steady_clock::now();

						info.proc();

						auto dur = std::chrono::steady_clock::now() - tp2;
						std::cout
							<< _bar(
								info.case_name + ": "
								+ std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count())
								+ " ms"
								, '-', 2)
							<< std::endl
						;

						if (&info != &li.back()) {
							std::cout << std::endl;
						}
					}

					auto dur = std::chrono::steady_clock::now() - tp;
					std::cout
						<< _bar(
							gn + ": "
							+ std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count())
							+ " ms"
							, '=', 4)
						<< std::endl
					;

					if (&li != &(--map.end())->second) {
						std::cout << std::endl;
					}
				}
			}

		private:
			struct _info_t {
				std::string case_name;
				std::function<void()> proc;
			};

			using _map_t = std::map<std::string, std::list<_info_t>>;

			static _map_t &_map() {
				static _map_t map;
				return map;
			}

			static std::string _bar(const std::string &c, char b, size_t sz) {
				std::stringstream ss;
				for (size_t i = 0; i < sz; ++i) {
					ss << b;
				}
				ss << " " << c;
				return ss.str();
			}
	};
}

#endif