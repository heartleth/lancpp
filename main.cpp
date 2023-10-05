#include "parse.hpp"
#include "dict.hpp"

using std::string_literals::operator""s;

int main() {
	lanc::flimsv dict_sv("dictionary.dic");
	lanc::flimsv lspec_sv("sample.lan");
	lanc::flimsv exp_sv("example.txt");
	auto lanspec = lspec_sv.lsv;
	auto examples = exp_sv.lsv;

	lanc::dict::full_dictionary dict(dict_sv.lsv);
	lanc::loadlan::ruleset rs(lanspec);
	rs.display();
	std::vector<lanc::limsv> noargs;

	auto [example, b] = examples.next_line();
	while (true) {
		std::cout << '<' << example.as_sv() << '>' << std::endl;
		auto [res, i] = lanc::parse::parse(example, lanc::parse::concrete_part(rs.part("main"s), rs, noargs), rs, dict);
		std::cout << res << std::endl;
		std::cout << i << std::endl;
		res->free_children();
		delete res;

		if (b.a == 0) {
			break;
		}
		else {
			auto nl = b.next_line();
			example = nl.first;
			b = nl.second;
		}
	}

	return 0;
}