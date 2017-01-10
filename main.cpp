#include <string>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

//===----------------------------------------------------------------------===//
// 词法分析
//===----------------------------------------------------------------------===//
enum Token {
	tok_eof = -1,
	tok_def = -2,
	tok_extern = -3,
	tok_identifier = -4,
	tok_number = -5
};
static std::string IdentifierStr; //tok_identifier
static double NumVal;             //tok_number

/// 返回下一个token
static int gettok() {
	static int LastChar = ' ';
	
	while (isspace(LastChar))
		LastChar = getchar();
	// def, extern, identifier
	if (isalpha(LastChar)) {
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getchar())))
			IdentifierStr += LastChar;

		if (IdentifierStr == "def") return tok_def;
		if (IdentifierStr == "extern") return tok_extern;
		return tok_identifier;
	}
	//数字
	if (isdigit(LastChar) || LastChar == '.') {
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');
		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}
	//注释
	if (LastChar == '#') {
		do LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok(); //跳过
	}

	if (LastChar == EOF)
		return tok_eof;
		
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

//===----------------------------------------------------------------------===//
// 语法分析
//===----------------------------------------------------------------------===//

int main() {
    
    return 0;
}