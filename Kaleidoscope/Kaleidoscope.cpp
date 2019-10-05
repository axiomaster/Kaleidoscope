#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <cstdlib>
#include <cstdio>
#include <map>
#include <vector>

using namespace llvm;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//
enum Token {
    tok_eof = -1,
    // commands
            tok_def = -2,
    tok_extern = -3,

    // primary
            tok_identifier = -4,
    tok_number = -5
};
static std::string IdentifierStr; //tok_identifier
static double NumVal;              //tok_number

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

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
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
        do
            LastChar = getchar();
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
namespace {
    class ExprAST {
    public:
        virtual ~ExprAST() = default;

        virtual Value *Codegen() = 0;
    };

    class NumberExprAST : public ExprAST {
        double Val;

    public:
        NumberExprAST(double val) : Val(val) {}

        virtual Value *Codegen();
    };

// 变量
    class VariableExprAST : public ExprAST {
        std::string Name;

    public:
        VariableExprAST(const std::string &name) : Name(name) {}

        virtual Value *Codegen();
    };

// 二元运算符
    class BinaryExprAST : public ExprAST {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs)
                : Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {}

        virtual Value *Codegen();
    };

// 函数调用表达式
    class CallExprAST : public ExprAST {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
                : Callee(callee), Args(std::move(args)) {}

        virtual Value *Codegen();
    };

// 函数原型
    class PrototypeAST {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &name, const std::vector<std::string> args)
                : Name(name), Args(std::move(args)) {}

        Function *Codegen();

        const std::string &getName() const {
            return Name;
        }
    };

    class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
                : Proto(std::move(proto)), Body(std::move(body)) {}

        Function *Codegen();
    };
} // namespace

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
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

/// Error
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return 0;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return 0;
}
//FunctionAST *LogErrorF(const char *Str) {
//	LogError(Str);
//	return 0;
//}

static std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = llvm::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

// 括号表达式 '('expression')'
/// parenexpr ::= '('expression')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); //'('

    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

// 变量引用： identifier
// 函数调用： identifier'('expression')'
/// identifierexpr
/// ::= identifier
/// ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken();

    if (CurTok != '(') //变量引用
        return llvm::make_unique<VariableExprAST>(IdName);
    // 函数调用
    getNextToken(); // eat '('
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                return nullptr;
            else
                Args.push_back(std::move(Arg));

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken(); // eat ')'
    return llvm::make_unique<CallExprAST>(IdName, Args);
}

// 一元表达式
/// primary
/// ::= identifier
/// ::= numberexpr
/// ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

/// binoprhs
/// ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();

        if (TokPrec < ExprPrec)
            return LHS;

        int BinOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (RHS == 0)
                return 0;
        }
        LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
/// ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

// 函数原型
/// prototype
/// ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')') // eat ')'
        return LogErrorP("Expected ')' in prototype");

    getNextToken();

    return llvm::make_unique<PrototypeAST>(FnName, ArgNames);
}

/// definition
/// ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); //eat def.
    auto Proto = ParsePrototype();
    if (Proto == 0)
        return 0;

    if (auto E = ParseExpression())
        return llvm::make_unique<FunctionAST>(Proto, E);
    return 0;
}

/// external
/// ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

// 顶层表达式
/// toplevelexpr
/// ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(Proto, E);
    }
    return 0;
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

Value *NumberExprAST::Codegen() {
    return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::Codegen() {
    Value *V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable name");
    return V;
}

Value *BinaryExprAST::Codegen() {
    Value *L = LHS->Codegen();
    Value *R = LHS->Codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
        case '+':
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFSub(L, R, "subtmp");
        case '*':
            return Builder.CreateFMul(L, R, "multmp");
        case '<':
            L = Builder.CreateFCmpULT(L, R, "cmptmp");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operatior");
    }
}

Value *CallExprAST::Codegen() {
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->Codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::Codegen() {
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function *FunctionAST::Codegen() {
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->Codegen();

    if (!TheFunction)
        return nullptr;

    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[Arg.getName()] = &Arg;

    if (Value *RetVal = Body->Codegen()) {
        Builder.CreateRet(RetVal);

        verifyFunction(*TheFunction);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        getNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        getNextToken();
    }
}

/// top
/// ::= definition | external | expression | ';'
static void MainLoop() {
    while (1) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
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

    TheModule = llvm::make_unique<Module>("my cool jit", TheContext);

    MainLoop();

    TheModule->dump();

    return 0;
}