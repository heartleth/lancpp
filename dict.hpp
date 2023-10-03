#ifndef __DICT_HPP__
#define __DICT_HPP__

#include "parse_decl.hpp"

namespace lanc::dict {
	class word {
	public:
		limsv text;
		std::vector<limsv> argv;
		word(std::vector<limsv>& words) {
			// auto words = line.words();
			text = words[0];
			argv.insert(argv.end(), words.begin() + 1, words.end());
		};
		word(const word& w) {
			text = w.text;
			argv = w.argv;
		}
		unsigned int len() {
			return (int)text.b - (int)text.a + 1;
		}
	};

	class full_dictionary {
	private:
		std::unordered_map<std::string, std::vector<word>> dict;
	public:
		void register_word(std::vector<limsv>& words, std::string& part) {
			if (dict.contains(part)) {
				dict[part].push_back(word(words));
			}
			else {
				dict.insert({ part, { word(words) } });
			}
		}

		full_dictionary(limsv lines) {
			auto [line, b] = lines.next_line();
			std::string part;
			while (true) {
				auto words = line.words();
				if (words.empty()) {
					goto gtcontinue_d;
				}
				if (*(char*)words[0].a == '#') {
					goto gtcontinue_d;
				}
				if (words[0] == "PART") {
					part = words[1].as_str();
				}
				else {
					register_word(words, part);
				}
			gtcontinue_d:
				if (b.a == 0) {
					break;
				}
				else {
					auto nl = b.next_line();
					line = nl.first;
					b = nl.second;
				}
			}
		}

		std::vector<word*> search_dict(std::string& part, limsv cond, limsv& s, char* reading, parse::expectation& expect, std::vector<limsv>& cargs) {
			std::vector<word*> ret;
			for (auto& word : dict[part]) {
				if (((char*)s.b - reading) < word.text.len()) {
					// ret.push_back(&word);
					continue;
				}
				if (!cond.is_some()) {
					if ((s >> reading).substr(0, word.text.len()) == word.text) {
						ret.push_back(&word);
					}
				}
				else {
					auto [c, next] = cond.split_once('&');
					while (true) {
						auto [arg, target] = c.split_once('=');
						bool neq = arg[-1] == '!';
						unsigned int argn = arg.substr(1, arg.len() - neq).to_n();
						bool ok = false;
						limsv arg_content;
						if (argn < word.argv.size()) {
							arg_content = word.argv[argn];
						}

						if (target[0] == ':') {
							ok = compare_arg(cargs, target.substr(1).to_n(), arg_content);
						}
						else if (target[0] == '@') {
							auto [ii, jj] = target.substr(1).split_once(':');
							ok = compare_arg(*expect.voca_attrs[ii.to_n()], jj.to_n(), arg_content);
						}
						else {
							if (arg_content.is_some()) {
								ok = target == arg_content;
							}
							else {
								ok = target == "0";
							}
						}

						if (!ok) {
							goto continue_for;
						}
						if (next.a == 0) {
							break;
						}
						else {
							auto split = next.split_once('&');
							c = split.first;
							next = split.second;
						}
					}
					if ((s >> reading).substr(0, word.text.len()) == word.text) {
						ret.push_back(&word);
					}
				}
				continue_for : 1;
			}

			return ret;
		}
	};
}

#endif 