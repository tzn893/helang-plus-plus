#pragma once
#include "common.h"
#include "tokens.h"
#include "llvm/IR/IRBuilder.h"


struct Declearation 
{
	string type;
	string name;
};


//gramma
//op    ::= +-*/.
//num   ::= num
//var   ::= id
//paren ::= ( expr )
//prim  ::= num | var | paren | call
//expr  ::= prim [op prim]* 
//call  ::= id([expr[,expr]*])
//body  ::= { [expr[;expr]*] }
//func  ::= fn id([id id[,id id]*]) body
//top   ::= [func]*

//every ast object should be derived from this class
class Expr 
{
	//for debuging
	u32			  line;
	u32			  start, end;
public:
	virtual ~Expr() {}
	virtual optional<llvm::Value*> CodeGenerate(string& error) = 0;

	Expr(Token& token) :line(token.line), start(token.start), end(token.end) {}
	string ErrorPrefix();
};

//currently we only support unsigned numbers
//num   ::= num
class NumberExpr : public Expr
{
	string num;
public:
	NumberExpr(string number,Token& token):num(number),Expr(token) {}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//var   ::= id
class VariableExpr : public Expr 
{
	string name;
public:
	VariableExpr(string name, Token& token): name(name),Expr(token)
	{
		he_assert(!name.empty());
	}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//expr  ::= prim [op prim]* 
class CalculateExpr : public Expr
{
	ptr<Expr> lhs, rhs;
	string op;
public:
	CalculateExpr(const string& op, ptr<Expr> lhs, ptr<Expr> rhs,Token& token) :
		op(op), lhs(lhs), rhs(rhs),Expr(token)
	{
		he_assert(lhs != nullptr && rhs != nullptr);
	}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//call  ::= id([expr[,expr]*])
class CallExpr : public Expr 
{
	string func;
	vector<ptr<Expr>> args;
public:
	CallExpr(const string& func, vector<ptr<Expr>>& args,Token& token) :func(func), args(args),Expr(token) {}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//assign ::=  id = expr;
class AssignExpr : public Expr {
	ptr<Expr> expr;
	string name;
public:
	AssignExpr(ptr<Expr> expr,const string& name,Token& token):Expr(token),expr(expr),name(name) {}
	//return nullptr if success,return nullopt if fails
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//declear ::= [mut] id assign
class DeclearExpr : public Expr {
	bool mut;
	string type, name;
	ptr<Expr> assign;
public:
	DeclearExpr(bool mut, const string& type, const string& name,ptr<Expr> expr, Token& token):Expr(token),
	type(type),name(name),mut(mut),assign(expr) {}
	//return nullptr if success,return nullopt if fail
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

class SignatureExpr : public Expr {
	string return_type, name;
	vector<Declearation> args;
public:
	optional<llvm::Function*> FunctionSignatureGenerate(string& error);
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
	SignatureExpr(const string& rt, const string& name, vector<Declearation>& args,Token& token):
	return_type(rt),name(name),args(args),Expr(token) {}

	string GetName() {return name;}
	string GetReturnType() { return return_type; }
	const vector<Declearation>& GetArgs() { return args; }
};


class BodyExpr : public Expr {
	vector<ptr<Expr>> body;
	//when expr == nullptr,body expr reach the end
	ptr<Expr> rt_expr;
public:
	BodyExpr(const vector<ptr<Expr>>& body, ptr<Expr> rt_expr, Token& token):body(body),rt_expr(rt_expr),Expr(token){}
	
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
	
	bool GenerateBodyCode(string& error,bool generate_return);
	bool HasReturnValue() { return rt_expr != nullptr; }
};

//ifexpr ::= if ( expr ) { body } [else {body} ]
class IfExpr : public Expr {
	ptr<BodyExpr> then_expr, else_expr;
	vector<ptr<BodyExpr>> elif_expr;
	vector<ptr<Expr>> elif_cond_expr;
	ptr<Expr> cond;
public:
	IfExpr(ptr<Expr> cond, ptr<BodyExpr> then_expr, ptr<BodyExpr> else_expr,
		vector<ptr<BodyExpr>>& elif_expr,vector<ptr<Expr>>& elif_cond_expr
		,Token& token) :
		then_expr(then_expr), else_expr(else_expr), cond(cond),Expr(token),
	elif_expr(elif_expr),elif_cond_expr(elif_cond_expr) {}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
};

//body  ::= [expr|assign|declear[;expr|assign|declear|nil]*] 
//func  ::= fn id([id id[,id id]*]) { body }
class FuncExpr : public Expr
{
	ptr<SignatureExpr> signature;
	ptr<BodyExpr> body;
public:
	FuncExpr(ptr<SignatureExpr> signature,ptr<BodyExpr> body,Token& token):
		signature(signature),body(body), Expr(token) {}
	virtual optional<llvm::Value*> CodeGenerate(string& error) override;

	optional<llvm::Function*> FunctionSignatureGenerate(string& error);
	bool FunctionBodyGenerate(string& error);
};

//top   ::= [func]*
class TopLevelExpr : public Expr 
{
	vector<ptr<FuncExpr>> funcs;
	vector<ptr<SignatureExpr>> extern_funcs;
public:
	TopLevelExpr(const vector<ptr<FuncExpr>>& funcs,
		vector<ptr<SignatureExpr>>& extern_funcs,
		Token& token):funcs(funcs),extern_funcs(extern_funcs), Expr(token) {}

	virtual optional<llvm::Value*> CodeGenerate(string& error) override;
	optional<string> IRGenerate(string& error);
};

class AST 
{
public:
	bool Parse(const vector<Token>& tokens);
	optional<string> GenerateIRCode();
	
	string ErrorMsg();
private:
	 ptr<TopLevelExpr> exprs;
	 string error;
};
