#ifndef __PARSE_HPP__
#define __PARSE_HPP__

#include "load_lan.hpp"
#include "dict.hpp"

namespace lanc::parse {
	std::pair<syntax_tree_node*, unsigned int> parse(limsv s, concrete_part part, loadlan::ruleset& rules, dict::full_dictionary& dictionary);
	std::pair<syntax_tree_node*, unsigned int> fit_rules(limsv s, limsv name, template_node* rule, loadlan::ruleset& rules, std::vector<limsv>& cargs, dict::full_dictionary& dictionary, bool is_trap = false) {
		auto expectations = nexts(name, rule, (char*)s.a);
		std::vector<std::pair<syntax_tree_node*, char*>> winners;
		unsigned int i = 0;

		while (!expectations.empty()) {
			for (auto& expect : expectations) {
				char* reading = expect.reading;
				auto rule = expect.rule;
				if (rule != 0) {
					auto prule = *rule;
					if (reading >= s.b) {
						expect.kill();
					}
					else {
						while (*expect.reading == ' ' || *expect.reading == '\r' || *expect.reading == '\t' || *expect.reading == '\n') {
							expect.read(1);
						}
						reading = expect.reading;
						if (prule.content->type == loadlan::template_type_enum::Text) {
							if (s >> reading >= prule.content->content[0]) {
								auto& text = prule.content->content[0];
								auto& label = prule.content->content[1];
								expect.read(text.len());
								expect.next_rule();
								expect.push_catrgory(new syntax_tree_node(label, text));
							}
							else {
								expect.kill();
							}
						}
						else if (prule.content->type == loadlan::template_type_enum::ShortPart) {
							auto part = prule.content->content[0].as_str();

							auto search_result = dictionary.search_dict(part, (prule.content->content.size() > 1) ? prule.content->content[1] : limsv(), s, reading, expect, cargs);
							if (search_result.empty()) {
								expect.kill();
							}
							else {
								dict::word* best_word = search_result[0];
								for (auto word : search_result) {
									if (word->text.len() > best_word->text.len()) {
										best_word = word;
									}
								}
								expect.push_catrgory(new syntax_tree_node(prule.content->content[0], best_word->text));
								expect.read(best_word->text.len());
								expect.register_attr(&best_word->argv);
								expect.next_rule();
							}
						}
						else if (prule.content->type == loadlan::template_type_enum::Template) {
							std::vector<limsv> t2;
							for (int j = 1; j < prule.content->content.size(); j++) {
								auto& e = prule.content->content[j];
								if (!e.is_some()) {
									t2.push_back(limsv());
								}
								else if (e[0] == '@') {
									auto [ns, ms] = e.substr(1).split_once(':');
									auto n = ns.to_n();
									auto m = ms.to_n();
									if (n < expect.voca_attrs.size()) {
										if (m < expect.voca_attrs[n]->size()) {
											t2.push_back((*expect.voca_attrs[n])[m]);
										}
										else {
											t2.push_back(limsv());
										}
									}
									else {
										t2.push_back(limsv());
									}
								}
								else {
									t2.push_back(e);
								}
							}
							auto& part = rules.part(prule.content->content[0]);
							concrete_part template_built(part, rules, t2);
							auto [new_tree, step] = parse(s >> reading, template_built, rules, dictionary);
							if (new_tree) {
								expect.read(step);
								expect.next_rule();
								expect.push_catrgory(new_tree);
							}
							else {
								expect.kill();
							}
						}
					}
				}
				else {
					winners.push_back({ expect.tree, reading });
					expect.tree_ownership = false;
					expect.kill();
				}
			}
			std::vector<expectation> new_expectations;
			for (auto& exp : expectations) {
				if (exp.is_alive) {
					auto&& enexts = exp.nexts();
					new_expectations.insert(new_expectations.end(), enexts.begin(), enexts.end());
				}
			}
			expectations = new_expectations;
		}

		if (winners.empty()) {
			return { 0, 0 };
		}
		else {
			std::pair<syntax_tree_node*, unsigned int> winner;
			unsigned int max = 0;
			for (auto& [t, i] : winners) {
				if (i - (char*)s.a > max) {
					winner.first = t;
					winner.second = max = i - (char*)s.a;
				}
			}

			return { winner.first, winner.second };
		}
	}

	std::pair<syntax_tree_node*, unsigned int> parse(limsv s, concrete_part part, loadlan::ruleset& rules, dict::full_dictionary& dictionary) {
		auto s2 = s.trim_front_iter();
		unsigned int trims = (char*)s2.a - (char*)s.a;
		syntax_tree_node* mp = 0;
		unsigned int m = trims;
		std::string rns;

		for (auto& [name, r] : part.rules) {
			auto [tr, x] = fit_rules(s2, name, r, rules, part.cargs, dictionary, part.is_trap);
			if (tr != 0) {
				if (x + trims >= m) {
					if (x + trims > m || (/* name < rns || */ rns.empty())) {
						mp = tr;
						m = x + trims;
						rns = name.as_str();
						rns.copy((char*)name.a, name.len());
					}
				}
			}
		}

		if (m == trims) {
			return { 0, 0 };
		}
		else {
			return { mp, m };
		}
	}
}

#endif