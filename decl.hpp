#ifndef __DECL_HPP__
#define __DECL_HPP__

#include <functional>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>
#include <string>
#include <vector>
#define minof(x, y) ((x) < (y) ? (x) : (y))

namespace lanc {
	struct pairhash {
	public:
		template <typename T, typename U>
		std::size_t operator()(const std::pair<T, U>& x) const {
			return std::hash<T>()(x.first) + std::hash<U>()(x.second);
		}
	};

	class limsv {
	public:
		char* str_end;
		void* a;
		void* b;

		limsv(char* start, char* last, char* end) :a(start), b(last), str_end(end) {}
		limsv(char* start, char* end) :a(start), b(end), str_end(end) {}
		limsv(const limsv& t) :a(t.a), b(t.b), str_end(t.str_end) {}
		limsv(char* end) :a(0), b(0), str_end(end) {}
		limsv() : a(0), b(0), str_end(0) {}
		std::string_view as_sv() const {
			return std::string_view((char*)a, (char*)b - (char*)a);
		}
		std::string as_str() const {
			return std::string((char*)a, (char*)b - (char*)a);
		}
		int to_n() const {
			if (len() == 1) {
				return *(char*)a - '0';
			}
			else {
				return atoi(std::string((char*)a, (char*)b).c_str());
			}
		}
		limsv substr(int s, int e = 99999) const {
			if (e >= 0) {
				return limsv((char*)minof((char*)a + s, (char*)b), (char*)minof((char*)a + e, (char*)b), str_end);
			}
			else {
				return limsv((char*)minof((char*)a + s, (char*)b), (char*)minof((char*)b + e, (char*)b), str_end);
			}
		}
		limsv trim_front_iter() const {
			int i = 0;
			if (len() == 0) {
				return *this;
			}
			while (((char*)a)[i] == ' ' || ((char*)a)[i] == '\t' || ((char*)a)[i] == '\r' || ((char*)a)[i] == '\n') {
				i += 1;
			}
			return substr(i);
		}
		limsv operator>>(char* na) const {
			return limsv(na, (char*)b, str_end);
		}
		unsigned int len() const {
			return (char*)b - (char*)a;
		}
		unsigned int max_len() const {
			return str_end - (char*)a;
		}
		void operator+= (const int l) {
			a = (char*)a + l;
			if (a > b) {
				a = b;
			}
		}
		bool is_some() const {
			return a != 0;
		}
		char operator[] (int i) const {
			if (i >= 0) {
				return ((char*)a)[i];
			}
			else {
				return ((char*)b)[i];
			}
		}
		std::pair<limsv, limsv> split_once(const char c) {
			for (char* iter = (char*)a; iter <= b; iter++) {
				if (*iter == c) {
					return { limsv((char*)a, iter, str_end), limsv(iter + 1, (char*)b, str_end) };
				}
			}
			return { *this, limsv(str_end) };
		}
		bool operator> (const char& t) const {
			char* p = (char*)a;
			for (; p <= (char*)b; p++) {
				if (*p == t) {
					return true;
				}
			}
			return false;
		}
		bool operator>= (const limsv& t) const {
			if (a == t.a && str_end == t.str_end) {
				return b >= t.b;
			}
			char* p = (char*)a;
			int i = 0;
			for (; p <= (char*)b; p++) {
				if (((char*)t.a)[i] != *p) {
					return false;
				}
				i += 1;
				if (i >= t.len()) {
					return true;
				}
			}
			return false;
		}
		std::pair<limsv, limsv> next_line() const {
			for (char* iter = (char*)a; iter <= b; iter++) {
				if (*iter == '\r') {
					return { limsv((char*)a, iter, str_end), limsv(iter + 2, (char*)b, str_end) };
				}
				else if (*iter == '\n') {
					return { limsv((char*)a, iter, str_end), limsv(iter + 1, (char*)b, str_end) };
				}
			}
			return { *this, limsv(str_end) };
		}
		std::vector<limsv> words() const {
			std::vector<limsv> res;
			char* lpos = (char*)a;
			for (char* iter = (char*)a; iter <= b; iter++) {
				if (*iter == ' ' || *iter == '\t') {
					if (iter > lpos) {
						res.push_back(limsv(lpos, iter, str_end));
					}
					lpos = iter + 1;
				}
			}
			if ((char*)b > lpos) {
				res.push_back(limsv(lpos, (char*)b, str_end));
			}
			return res;
		}
		bool operator==(const char* s) const {
			char* iter = (char*)a;
			for (; iter < b; iter++) {
				if (*iter != s[iter - (char*)a]) {
					return false;
				}
			}
			if (iter == b) {
				return true;
			}
			return false;
		}
		bool operator==(const limsv& s) const {
			char* iter = (char*)a;
			if (s.len() != len()) {
				return false;
			}
			for (; iter < b; iter++) {
				if (*iter != s[iter - (char*)a]) {
					return false;
				}
			}
			return iter == b;
		}
	};

	class flimsv {
	private:
		char* c_ptr;
	public:
		limsv lsv;
		flimsv(const std::string& filename) {
			std::ifstream file(filename);
			std::stringstream buff;
			buff << file.rdbuf();
			auto bufstr = buff.str();
			c_ptr = new char[bufstr.length() + 1];
			std::strcpy(c_ptr, bufstr.c_str());
			lsv = limsv(c_ptr, c_ptr + bufstr.length());
		}
		~flimsv() {
			delete c_ptr;
		}
	};

	bool compare_arg(std::vector<limsv>& a, unsigned int i, limsv b) {
		if (b.is_some()) {
			if (i < a.size()) {
				return b == a[i];
			}
			else {
				return b == "0";
			}
		}
		else {
			if (i < a.size()) {
				return a[i] == "0";
			}
			else {
				return true;
			}
		}
	}
}

#endif