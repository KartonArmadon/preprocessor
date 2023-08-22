#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;
namespace fs = std::filesystem;

const static regex header_reg(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
const static regex library_reg(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
const static regex test_reg(R"/(include)/");

path operator""_p(const char*, std::size_t);

// напишите эту функцию
bool Preprocess(const path&, const path&, const vector<path>&);

bool RecursivePreprocess(ifstream&, ofstream&, const path&, const vector<path>&);

bool findInclude(const string&, const vector<path>&, path& result);

string GetFileContents(string);

string ReadLine(istream&);

void Test() {
	error_code err;
	filesystem::remove_all("sources"_p, err);
	filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
	filesystem::create_directories("sources"_p / "include1"_p, err);
	filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

	{
		ofstream file("sources/a.cpp");
		file << "// this comment before include\n"
			"#include \"dir1/b.h\"\n"
			"// text between b.h and c.h\n"
			"#include \"dir1/d.h\"\n"
			"\n"
			"int SayHello() {\n"
			"    cout << \"hello, world!\" << endl;\n"
			"#   include<dummy.txt>\n"
			"}\n"sv;
	}
	{
		ofstream file("sources/dir1/b.h");
		file << "// text from b.h before include\n"
			"#include \"subdir/c.h\"\n"
			"// text from b.h after include"sv;
	}
	{
		ofstream file("sources/dir1/subdir/c.h");
		file << "// text from c.h before include\n"
			"#include <std1.h>\n"
			"// text from c.h after include\n"sv;
	}
	{
		ofstream file("sources/dir1/d.h");
		file << "// text from d.h before include\n"
			"#include \"lib/std2.h\"\n"
			"// text from d.h after include\n"sv;
	}
	{
		ofstream file("sources/include1/std1.h");
		file << "// std1\n"sv;
	}
	{
		ofstream file("sources/include2/lib/std2.h");
		file << "// std2\n"sv;
	}

	assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
		{ "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

	ostringstream test_out;
	test_out << "// this comment before include\n"
		"// text from b.h before include\n"
		"// text from c.h before include\n"
		"// std1\n"
		"// text from c.h after include\n"
		"// text from b.h after include\n"
		"// text between b.h and c.h\n"
		"// text from d.h before include\n"
		"// std2\n"
		"// text from d.h after include\n"
		"\n"
		"int SayHello() {\n"
		"    cout << \"hello, world!\" << endl;\n"sv;

	assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
	Test();
}

path operator""_p(const char* data, std::size_t sz) {
	return path(data, data + sz);
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {

	ifstream in_stream(in_file);
	//Если файл не открылся
	if (!in_stream.is_open())
		return false;
	//Если файл открылся
	ofstream out_stream(out_file);
	if (!out_stream.is_open())
		return false;
	return RecursivePreprocess(in_stream, out_stream, in_file, include_directories);
}

bool RecursivePreprocess(
	ifstream& in,
	ofstream& out,
	const path& p,
	const vector<path>& include_directories) {
	if (!in.is_open()) {
		return false;
	}
	string current_str;
	smatch match_header;
	smatch match_library;
	int current_str_number = 0; //номер строки
	while (getline(in, current_str)) {
		current_str_number++;
		if (regex_match(current_str, match_header, header_reg) ||
			regex_match(current_str, match_library, library_reg)) {
			ifstream include_file;
			path relative_path;
			path global_path;
			if (!match_header.empty()) {
				relative_path = string(match_header[1]);
				global_path = p.parent_path() / relative_path;
			}
			else {
				relative_path = string(match_library[1]);
			}

			bool isIncludeExists = fs::exists(global_path);
			if (!isIncludeExists) {
				isIncludeExists = findInclude(relative_path.string(), include_directories, global_path);
			}

			if (isIncludeExists) {
				ifstream lib_file(global_path);
				RecursivePreprocess(lib_file, out, global_path, include_directories);
			}
			else {
				cout << "unknown include file "s << relative_path.string() << " at file "s << p.string() << " at line "s << current_str_number << endl;
				return false;
			}
		}
		else
			out << current_str << endl;
	}
	return true;
}

bool findInclude(const string& file_to_find, const vector<path>& include_directories, path& result) {
	for (const auto& item : include_directories) {
		result = item / file_to_find;
		if (fs::exists(result))
			return true;
	}
	result.clear();
	return false;
}

string GetFileContents(string file) {
	ifstream stream(file);

	// конструируем string по двум итераторам
	return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

string ReadLine(istream& stream) {
	string s;
	getline(stream, s);
	return s;
}