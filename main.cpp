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
// AST
//===----------------------------------------------------------------------===//
class ExprAST {
public:
	virtual ~ExprAST(){}
};

class NumberExprAST :public ExprAST {
	double Val;
public:
	NumberExprAST(double val):Val(val){}
};

// 变量
class VariableExprAST :public ExprAST {
	std::string Name;
public:
	VariableExprAST(const std::string &name):Name(name){}
};

// 运算符
class BinaryExprAST :public ExprAST {
	char Op;
	ExprAST *LHS, *RHS;
public:
	BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
		:Op(op), LHS(lhs), RHS(rhs){}
};

// 函数调用？？？
class CallExprAST :public ExprAST {
	std::string Callee;
	std::vector<ExprAST*> Args;
public:
	CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
		: Callee(callee), Args(args){}
};

// 函数原型
class PrototypeAST {
	std::string Name;
	std::vector<std::string> Args;
public:
	PrototypeAST(const std::string &name, const std::vector<std::string> &args)
		:Name(name), Args(args){}
};

class FunctionAST {
	PrototypeAST *Proto;
	ExprAST *Body;
public:
	FunctionAST(PrototypeAST *proto, ExprAST *body)
		:Proto(proto), Body(body){}
};

//===----------------------------------------------------------------------===//
// parser
//===----------------------------------------------------------------------===//

static int CurTok;
static int getNextToken() { //跳过一个token
	return CurTok = gettok();
}

static std::map<char, int> BinopPrecedence;
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;

	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

/// Error
ExprAST *Error(const char *Str) {
	fprintf(stderr, "Error: %s\n", Str);
	return 0;
}
PrototypeAST *ErrorP(const char *Str) {
	Error(Str);
	return 0;
}
FunctionAST *ErrorF(const char *Str) {
	Error(Str);
	return 0;
}

static ExprAST *ParseExpression();

/// numberexpr ::= number
static ExprAST *ParseNumberExpr() {
	ExprAST *Result = new NumberExprAST(NumVal);
	getNextToken();
	return Result;
}

// 括号表达式 '('expression')'
/// parenexpr ::= '('expression')'
static ExprAST *ParseParenExpr() {
	getNextToken(); //'('
	
	ExprAST *V = ParseExpression();
	if (!V) return 0;
	
	if (CurTok != ')')
		return Error("expected ')'");
	getNextToken();
	return V;
}

// 变量引用： identifier
// 函数调用： identifier'('expression')'
/// identifierexpr
/// ::= identifier
/// ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken();
	
	if (CurTok != '(') //变量引用
		return new VariableExprAST(IdName);
	// 函数调用
	getNextToken(); // eat '('
	std::vector<ExprAST*> Args;
	if (CurTok != ')') {
		while (1) {
			ExprAST *Arg = ParseExpression();
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (CurTok == ')') break;

			if (CurTok != ',')
				return Error("Expected ')' or ',' in argument list");
			getNextToken();
		}
	}

	getNextToken(); // eat ')'
	return new CallExprAST(IdName, Args);
}

// 一元表达式
/// primary
/// ::= identifier
/// ::= numberexpr
/// ::= parenexpr
static ExprAST *ParsePrimary() {
	switch (CurTok)
	{
	default: return Error("unknown token when expecting an expression");
	case tok_identifier: return ParseIdentifierExpr();
	case tok_number: return ParseNumberExpr();
	case '(': return ParseParenExpr();
	}
}

/// binoprhs
/// ::= ('+' primary)*
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS)
{
	while (1) {
		int TokPrec = GetTokPrecedence();

		if (TokPrec < ExprPrec)
			return LHS;

		int BinOp = CurTok;
		getNextToken();

		ExprAST *RHS = ParsePrimary();
		if (!RHS) return 0;

		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, RHS);
			if (RHS == 0) return 0;
		}
		LHS = new BinaryExprAST(BinOp, LHS, RHS);
	}
}

/// expression
/// ::= primary binoprhs
static ExprAST *ParseExpression() {
	ExprAST *LHS = ParsePrimary();
	if (!LHS) return 0;
	
	return ParseBinOpRHS(0, LHS);
}
// 函数原型
/// prototype
/// ::= id '(' id* ')'
static PrototypeAST *ParsePrototype() {
	if (CurTok != tok_identifier)
		return ErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(')
		return ErrorP("Expected '(' in prototype");

	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')') // eat ')'
		return ErrorP("Expected ')' in prototype");

	getNextToken();

	return new PrototypeAST(FnName, ArgNames);
}

/// definition
/// ::= 'def' prototype expression
static FunctionAST *ParseDefinition() {
	getNextToken(); //eat def.
	PrototypeAST *Proto = ParsePrototype();
	if (Proto == 0) return 0;

	if (ExprAST *E = ParseExpression())
		return new FunctionAST(Proto, E);
	return 0;
}

/// external
/// ::= 'extern' prototype
static PrototypeAST *ParseExtern() {
	getNextToken();
	return ParsePrototype();
}

// 顶层表达式
/// toplevelexpr
/// ::= expression
static FunctionAST *ParseTopLevelExpr() {
	if (ExprAST *E = ParseExpression()) {
		PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
		return new FunctionAST(Proto, E);
	}
	return 0;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleDefinition()
{
	if (ParseDefinition()) {
		fprintf(stderr, "Parsed a function definition.\n");
	}
	else {
		getNextToken();
	}
}

static void HandleExtern()
{
	if (ParseExtern()) {
		fprintf(stderr, "Parsed an extern\n");
	}
	else {
		getNextToken();
	}
}

static void HandleTopLevelExpression()
{
	if (ParseTopLevelExpr()) {
		fprintf(stderr, "Parsed a top-level expr\n");
	}
	else {
		getNextToken();
	}
}

/// top
/// ::= definition | external | expression | ';'
static void MainLoop() {
	while (1) {
		fprintf(stderr, "ready> ");
		switch (CurTok)
		{
		case tok_eof:
			return;
		case ';':
			getNextToken();
			break;
		case tok_def:
			HandleDefinition();
			break;
		case tok_extern:
			HandleExtern();
			break;
		default:
			HandleTopLevelExpression();
			break;
		}
	}
}


int main() {

	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;

	fprintf(stderr, "ready> ");
	getNextToken();
	MainLoop();
    
    return 0;
}