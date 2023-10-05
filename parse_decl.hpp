#ifndef __PARSE_DECL_HPP__
#define __PARSE_DECL_HPP__

#include "load_lan.hpp"

namespace lanc::parse {
	using loadlan::template_node;
	using loadlan::template_item;
	class syntax_tree_node {
	public:
		static constexpr bool Category = 1;
		static constexpr bool Vocab = 0;

		std::vector<syntax_tree_node*> children;
		limsv label;
		limsv text;
		bool type;

		syntax_tree_node(limsv p = limsv(), limsv t = limsv()) :label(p), text(t), type(0) {}
		syntax_tree_node(syntax_tree_node& cp) {
			label = cp.label;
			text = cp.text;
			children.reserve(cp.children.size());
			for (auto c : cp.children) {
				children.push_back(new syntax_tree_node(*c));
			}
		}
		void push_category(syntax_tree_node* node) {
			children.push_back(node);
		}
		void free_children() {
			for (auto c : children) {
				// std::cout << text.as_sv() << " " << label.as_sv() << std::endl;
				c->free_children();
				delete c;
			}
			children.clear();
		}
		inline bool is_category() const { return type; }
		inline bool is_vocab() const { return !type; }
	};

	class expectation {
	public:
		std::vector<std::vector<limsv>*> voca_attrs;
		template_node* rule;
		syntax_tree_node* tree;
		bool tree_ownership;
		char* reading;
		bool is_alive;
		limsv name;

		expectation(limsv nm, template_node* prule, syntax_tree_node* t, char* rd) {
			tree_ownership = true;
			is_alive = true;
			reading = rd;
			rule = prule;
			name = nm;
			tree = t;
		}
		void kill() {
			is_alive = false;
			if (tree_ownership) {
				tree->free_children();
				delete tree;
			}
		}
		void read(unsigned int l) {
			reading += l;
		}
		void register_attr(std::vector<limsv>* attr) {
			voca_attrs.push_back(attr);
		}
		void push_catrgory(syntax_tree_node* stn) {
			tree->push_category(stn);
		}
		std::vector<expectation> nexts() {
			std::vector<expectation> ret;
			if (rule == 0) {
				ret.push_back(*this);
				return ret;
			}
			unsigned int i = 0;
			auto iter = rule->begin();
			auto d = iter;
			for (; iter.ptr != 0; ++iter) {
				auto& r = *iter.ptr;
				if (r.is_optional || r.is_free) {
					auto p = expectation(name, rule->step(i), new syntax_tree_node(*tree), reading);
					p.voca_attrs.insert(p.voca_attrs.end(), voca_attrs.begin(), voca_attrs.end());
					ret.push_back(p);
				}
				else {
					auto p = expectation(name, rule->step(i), new syntax_tree_node(*tree), reading);
					p.voca_attrs.insert(p.voca_attrs.end(), voca_attrs.begin(), voca_attrs.end());
					ret.push_back(p);
					tree_ownership = false;
					return ret;
				}
				d = iter;
				i += 1;
			}
			if (d.ptr->is_optional) {
				auto p = expectation(name, 0, tree, reading);
				ret.push_back(p);
				tree_ownership = false;
			}
			return ret;
		}
		void next_rule() {
			if (!rule->is_free) {
				rule = (template_node*)rule->next;
			}
		}
	};

	struct concrete_part {
		std::unordered_map<std::string, template_node*> vals;
		std::vector<std::pair<limsv, template_node*>> rules;
		std::vector<limsv> cargs;
		bool is_trap;
		limsv part;
		limsv id;
		template_node* argrules(template_node* rule) {
			auto head = new template_node(0);
			auto tail = head;
			template_node* iter = rule;

			for (iter = rule; iter != 0; iter = (template_node*)iter->next) {
				template_node r = *iter;
				bool qualified = true;
				if (r.condition.is_some()) {
					auto [param, value] = r.condition.split_once('=');
					unsigned int param_index = 0;
					bool is_not = false;
					if (param[-1] == '!') {
						is_not = true;
						param_index = param.substr(1, -1).to_n();
					}
					else {
						is_not = false;
						param_index = param.substr(1).to_n();
					}
					qualified = compare_arg(cargs, param_index, value) ^ is_not;
					// if (param_index >= cargs.size()) {
					// 	qualified = is_not ^ (value == "0");
					// }
					// else {
					// 	qualified = is_not ^ (value == cargs[param_index]);
					// }
				}
				if (qualified) {
					if (r.content->type == loadlan::template_type_enum::Embed) {
						// auto embed_ptr = vals[r.content->content[0].as_str()];
						// tail->next = embed_ptr;
						// while (tail->next != 0) {
						//     if (tail->next != 0) {
						//         if (((template_node*)tail->next)->is_eov) {

						//             break;
						//         }
						//     }
						//     tail = (template_node*)tail->next;
						auto embed_tail = vals[r.content->content[0].as_str()];
						while (embed_tail->next != 0) {
							tail = (template_node*)(tail->next = new template_node(*embed_tail));
							embed_tail = (template_node*)embed_tail->next;
						}
						tail = (template_node*)(tail->next = new template_node(*embed_tail));
					}
					else if (r.content->type == loadlan::template_type_enum::Template) {
						std::vector<limsv> concreteargs;
						for (int i = 1; i < r.content->content.size(); i++) {
							if (r.content->content[i][0] == ':') {
								unsigned int idx = r.content->content[i].to_n();
								if (cargs.size() <= idx) {
									concreteargs.push_back(limsv());
								}
								else {
									concreteargs.push_back(cargs[idx]);
								}
							}
							else {
								concreteargs.push_back(r.content->content[i]);
							}
						}
						tail = (template_node*)(tail->next = new template_node(
							new template_item(loadlan::template_type<loadlan::template_type_enum::Template>(), r.content->content[0], concreteargs),
							r.is_optional, r.is_free, r.condition
						));
					}
					else {
						tail = (template_node*)(tail->next = new template_node(r));
					}
				}
			}
			tail->next = 0;
			auto n = (template_node*)head->next;
			delete head;
			return n;
		}
		concrete_part(loadlan::ruleset::phrase_rule& prr, loadlan::ruleset& rset, std::vector<limsv>& args) {
			cargs = args;
			is_trap = false;
			for (auto& pr : prr.children) {
				if (pr.type == loadlan::ruleset::phrase_rule::Rule) {
					auto r = (template_node*)pr.tnode;
					rules.push_back({ pr.name_or_value, argrules(r) });
				}
				else if (pr.type == loadlan::ruleset::phrase_rule::SetVariable) {
					vals.insert({ pr.name_or_value.as_str(), (template_node*)pr.tnode });
				}
				else if (pr.type == loadlan::ruleset::phrase_rule::If) {
					if (compare_arg(args, pr.param, pr.name_or_value) ^ pr.is_trap_or_unless) {
						auto r2 = concrete_part(pr, rset, args);
						rules.insert(rules.end(), r2.rules.begin(), r2.rules.end());
					}
					// if (args.size() <= pr.param) {
					// 	if (pr.name_or_value == "0" ^ pr.is_trap_or_unless) {
					// 		auto r2 = concrete_part(pr, rset, args);
					// 		rules.insert(rules.end(), r2.rules.begin(), r2.rules.end());
					// 	}
					// }
					// else {
					// 	if (args[pr.param].as_sv() == pr.name_or_value.as_sv() ^ pr.is_trap_or_unless) {
					// 		auto r2 = concrete_part(pr, rset, args);
					// 		rules.insert(rules.end(), r2.rules.begin(), r2.rules.end());
					// 	}
					// }
				}
			}
		}
	};

	std::vector<expectation> nexts(limsv name, template_node* rule, char* na) {
		std::vector<expectation> ret;
		unsigned int i = 0;
		auto iter = rule->begin();
		auto d = iter;
		for (; iter.ptr != 0; ++iter) {
			auto& r = *iter.ptr;
			if (r.is_optional || r.is_free) {
				auto p = expectation(name, rule->step(i), new syntax_tree_node(name), na);
				ret.push_back(p);
			}
			else {
				auto p = expectation(name, rule->step(i), new syntax_tree_node(name), na);
				ret.push_back(p);
				return ret;
			}
			d = iter;
			i += 1;
		}
		if (iter.ptr == 0) {
			ret.push_back(expectation(name, 0, new syntax_tree_node(name), na));
		}
		return ret;
	}
}

#endif