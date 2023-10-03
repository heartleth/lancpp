#ifndef __LOAD_LAN_HPP__
#define __LOAD_LAN_HPP__

#include "decl.hpp"
#include <unordered_map>
#include <stack>

namespace lanc::loadlan {
	enum class template_type_enum {
		Embed, Text, ShortPart, Template
	};

	template<template_type_enum t>
	struct template_type {
		const template_type_enum type = t;
		using Embed = template_type<template_type_enum::Embed>();
		using Text = template_type<template_type_enum::Text>();
		using ShortPart = template_type<template_type_enum::ShortPart>();
		using Template = template_type<template_type_enum::Template>();
		template_type() {}
	};

	class template_item {
		// private:
	public:
		std::vector<limsv> content;
		template_type_enum type;
		template_item(template_type<template_type_enum::Embed> t, limsv text) {
			type = template_type_enum::Embed;
			content.push_back(text);
		}
		template_item(template_type<template_type_enum::Text> t, limsv text, limsv label) {
			type = template_type_enum::Text;
			content.push_back(text);
			content.push_back(label);
		}
		template_item(template_type<template_type_enum::ShortPart> t, limsv pos) {
			type = template_type_enum::ShortPart;
			content.push_back(pos);
		}
		template_item(template_type<template_type_enum::ShortPart> t, limsv pos, limsv backcond) {
			type = template_type_enum::ShortPart;
			content.push_back(pos);
			content.push_back(backcond);
		}
		template_item(template_type<template_type_enum::Template> t, limsv pos, std::vector<limsv> args) {
			type = template_type_enum::Template;
			content.push_back(pos);
			// content.insert(content.end(), args.begin(), args.end());
			for (auto& k : args) {
				content.push_back(k);
			}
		}
	};

	class template_node {
	public:
		template_item* content;
		bool is_eov = false;
		bool is_optional;
		limsv condition;
		void* next = 0;
		bool is_free;

		template_node(template_item* temp, bool opt = false, bool fr = false, limsv cond = limsv()) {
			is_optional = opt;
			condition = cond;
			content = temp;
			is_eov = false;
			is_free = fr;
			next = 0;
		}

		void free_nexts() {
			if (next != 0) {
				((template_node*)next)->free_nexts();
				delete ((template_node*)next)->content;
				delete (template_node*)next;
			}
		}

		template_node* step(int n) {
			return (iterator(this) + n).ptr;
		}

		struct iterator {
			template_node* ptr;
			template_node& operator*() const {
				return *ptr;
			}
			iterator operator++() {
				ptr = (template_node*)ptr->next;
				return *this;
			}
			iterator operator+(int n) {
				template_node* ret = ptr;
				for (int i = 0; i < n; i++) {
					ptr = (template_node*)ptr->next;
				}
				return iterator(ptr);
			}
			bool operator!=(iterator& i) {
				return ptr != i.ptr;
			}
			iterator(template_node* p) :ptr(p) {}

		};
		iterator begin() {
			return iterator(this);
		}
		iterator end() {
			return iterator(0);
		}
		void display() {
			for (auto c : *this) {
				auto& content = c.content;
				std::cout << (int)content->type << ";";
				for (auto& cont : content->content) {
					std::cout << '<' << cont.as_sv() << "> ";
				}
				std::cout << "/ ";
			}
			std::cout << std::endl;
		}
	};

	class ruleset {
	private:
		enum class phrase_rule_type { SetVariable, Phrase, Rule, If };
	public:
		struct phrase_rule {
			using enum phrase_rule_type;
			limsv name_or_value;
			phrase_rule_type type;
			std::vector<phrase_rule> children;
			bool is_trap_or_unless;
			unsigned int param = 0;
			void* tnode = 0;

			void free_tnode() {
				if (tnode) {
					((template_node*)tnode)->free_nexts();
					delete ((template_node*)tnode)->content;
					delete (template_node*)tnode;
				}
				for (auto& child : children) {
					child.free_tnode();
				}
			}
			void display(int indents = 0) {
				for (int i = 0; i < indents; i++) {
					std::cout << "    ";
				}
				if (tnode) {
					std::cout << name_or_value.as_sv() << ": ";
					auto iter = (template_node*)tnode;
					for (auto c : *iter) {
						auto& content = c.content;
						std::cout << (int)content->type << ";";
						for (auto& cont : content->content) {
							std::cout << '<' << cont.as_sv() << "> ";
						}
						std::cout << "/ ";
					}
					// while (iter != 0 && i < 10) {

					// }
					std::cout << std::endl;
				}
				else if (type == If) {
					std::cout << "If " << param << ' ' << name_or_value.as_sv() << std::endl;
					for (auto& child : children) {
						child.display(indents + 1);
					}
				}
				else if (type == Phrase) {
					std::cout << "PART " << name_or_value.as_sv() << std::endl;;
					for (auto& child : children) {
						child.display(indents + 1);
					}
				}
			}
		};
	private:
		using phr_vec = std::vector<phrase_rule>;
		std::unordered_map<std::string, phrase_rule> map;
		static phrase_rule parse_rule(std::vector<limsv>& words) {
			auto head = new template_node(0);
			std::vector<limsv> args;
			bool inargs = false;
			bool aropt = false;
			auto tail = head;
			limsv name;
			limsv cond;
			limsv pos;

			for (auto iter = words.begin() + 2; iter != words.end(); iter++) {
				auto sword = *iter;
				if (inargs) {
					if (sword == ")") {
						inargs = false;
						tail = (template_node*)(tail->next = new template_node(
							new template_item(template_type<template_type_enum::Template>(), pos, args),
							aropt, false, cond
						));
						args.clear();
						cond = limsv();
					}
					else {
						args.push_back(sword);
					}
				}
				else {
					if (sword[0] == '$') {
						if (sword[-1] == '(') {
							inargs = true;
							aropt = false;
							pos = sword.substr(1, -1);
						}
						else {
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::Template>(), sword.substr(1), args),
								false, false, cond
							));
						}
					}
					else if (sword[0] == '*') {
						if (sword > '[') {
							auto [part, dcond] = sword.split_once('[');
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::ShortPart>(), part.substr(1), dcond.substr(0, -1)),
								false, false, cond
							));
						}
						else {
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::ShortPart>(), sword.substr(1)),
								false, false, cond
							));
						}
					}
					else if (sword[0] == '%') {
						tail = (template_node*)(tail->next = new template_node(
							new template_item(template_type<template_type_enum::Embed>(), sword.substr(1))
						));
					}
					else if (sword[0] == '?') {
						if (sword[1] == '$') {
							if (sword[-1] == '(') {
								inargs = true;
								aropt = true;
								pos = sword.substr(2, -1);
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::Template>(), sword.substr(2), args),
									true, false, cond
								));
							}
						}
						else if (sword[1] == '*') {
							if (sword > '[') {
								auto [part, dcond] = sword.split_once('[');
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), part.substr(2), dcond.substr(0, -1)),
									true, false, cond
								));
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), sword.substr(2)),
									true, false, cond
								));
							}
						}
						else {
							auto [text, label] = sword.split_once('-');
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::Text>(), text.substr(1), label),
								true, false, cond
							));
						}
					}
					else if (sword[1] == '>') {
						if (sword[2] == '$') {
							if (sword[-1] == '(') {
								inargs = true;
								aropt = true;
								pos = sword.substr(3, -1);
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::Template>(), sword.substr(3), args),
									false, true, cond
								));
								if (sword[0] == '|') {
									tail = (template_node*)(tail->next = new template_node(
										new template_item(template_type<template_type_enum::Template>(), sword.substr(3), args),
										false, false, cond
									));
								}
							}
						}
						else if (sword[2] == '*') {
							if (sword > '[') {
								auto [part, dcond] = sword.split_once('[');
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), part.substr(3), dcond.substr(0, -1)),
									false, true, cond
								));
								if (sword[0] == '|') {
									tail = (template_node*)(tail->next = new template_node(
										new template_item(template_type<template_type_enum::ShortPart>(), part.substr(3), dcond.substr(0, -1)),
										false, false, cond
									));
								}
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), sword.substr(3)),
									false, true, cond
								));
								if (sword[0] == '|') {
									tail = (template_node*)(tail->next = new template_node(
										new template_item(template_type<template_type_enum::ShortPart>(), sword.substr(3)),
										false, false, cond
									));
								}
							}
						}
						else {
							auto [text, label] = sword.split_once('-');
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::Text>(), text.substr(2), label),
								false, true, cond
							));
							if (sword[0] == '|') {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::Text>(), text.substr(2), label),
									false, false, cond
								));
							}
						}
					}
					else if (sword[0] == '[') {
						auto splited = sword.split_once(']');
						cond = splited.first.substr(1);
						auto rword = splited.second;
						if (rword[0] == '$') {
							if (rword[-1] == '(') {
								inargs = true;
								aropt = false;
								pos = rword.substr(1, -1);
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::Template>(), rword.substr(1), args),
									false, false, cond
								));
							}
						}
						else if (rword[0] == '*') {
							if (rword > '[') {
								auto [part, dcond] = rword.split_once('[');
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), part.substr(1), dcond.substr(0, -1)),
									false, false, cond
								));
							}
							else {
								tail = (template_node*)(tail->next = new template_node(
									new template_item(template_type<template_type_enum::ShortPart>(), rword.substr(1)),
									false, false, cond
								));
							}
						}
						else {
							auto [text, label] = rword.split_once('-');
							tail = (template_node*)(tail->next = new template_node(
								new template_item(template_type<template_type_enum::Text>(), text, label),
								false, false, cond
							));
						}
					}
					else {
						auto [text, label] = sword.split_once('-');
						tail = (template_node*)(tail->next = new template_node(
							new template_item(template_type<template_type_enum::Text>(), text, label),
							false, false, cond
						));
					}
				}
			}
			tail->next = 0;
			auto n = head->next;
			delete head;
			return {
				words[1],
				phrase_rule::Rule,
				std::vector<phrase_rule>(),
				0, 0,
				n
			};
		}
	public:
		ruleset(limsv lines) {
			auto [a, b] = lines.next_line();
			std::stack<phrase_rule> st;
			while (true) {
				auto words = a.words();
				if (words.empty()) {
					goto gtcontinue;
				}
				if (*(char*)words[0].a == '#') {
					goto gtcontinue;
				}
				if (words[0] == "PART") {
					st.push({ words[1], phrase_rule::Phrase, phr_vec(), false });
				}
				else if (words[0] == "TRAP") {
					st.push({ words[1], phrase_rule::Phrase, phr_vec(), true });
				}
				else if (words[0] == "RULE") {
					auto node = parse_rule(words);
					st.top().children.push_back(node);
				}
				else if (words[0] == "SET") {
					auto node = parse_rule(words);
					node.type = phrase_rule::SetVariable;
					st.top().children.push_back(node);
				}
				else if (words[0] == "IF") {
					unsigned int argn = words[1].substr(1).to_n();
					st.push({ words[2], phrase_rule::If, phr_vec(), false, argn });
				}
				else if (words[0] == "UNLESS") {
					unsigned int argn = words[1].substr(1).to_n();
					st.push({ words[2], phrase_rule::If, phr_vec(), true, argn });
				}
				else if (words[0] == "END") {
					auto p = st.top();
					st.pop();
					if (p.type == phrase_rule::Phrase) {
						auto key = p.name_or_value.as_str();
						map.insert({ key, p });
					}
					else if (p.type == phrase_rule::If) {
						st.top().children.push_back(p);
					}
				}

			gtcontinue:
				if (b.a == 0) {
					break;
				}
				else {
					auto nl = b.next_line();
					a = nl.first;
					b = nl.second;
				}
			}
		}
		void display() {
			for (auto& [k, v] : map) {
				v.display();
			}
		}
		phrase_rule& part(limsv& key) {
			return map[key.as_str()];
		}
		phrase_rule& part(const std::string&& key) {
			return map[key];
		}
		~ruleset() {
			for (auto& [key, phr] : map) {
				phr.free_tnode();
			}
		}
	};
}

#endif