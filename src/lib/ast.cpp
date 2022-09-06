#include "ast.h"
#include <unordered_map>
#include <stack>

extern const char* g_token_type_name_table[HE_TOKEN_COUNT];
extern unordered_map<string, u32> g_operator_priority;



class ASTParser 
{
public:
	ASTParser(const vector<Token>& tokens):tokens(tokens) {}

	optional<ptr<TopLevelExpr>> Parse(string& error);

private:
	optional<ptr<SignatureExpr>> ParseExtern(u32 start,u32& end,string& error);
	optional<ptr<FuncExpr>> ParseFunc(u32 start, u32& end,string& error);
	optional<ptr<SignatureExpr>> ParseSignature(u32 start, u32& end,string& error);
	optional<ptr<IfExpr>>   ParseIf(u32 start, u32& end, string& error);
	optional<ptr<BodyExpr>>	ParseBody(u32 start,u32 end,string& error);
	optional<ptr<Expr>>     ParseExpression(u32 start,u32 end,string& error);
	optional<ptr<Expr>>		ParseBinExpression(u32 start,u32 end,ptr<Expr> lhs,string& error);
	optional<ptr<Expr>>		ParsePrimExpression(u32 start,u32 end,string& error);
	optional<ptr<DeclearExpr>>	  ParseDeclear(u32 start, u32 end, string& error);
	optional<ptr<AssignExpr>>     ParseAssign(u32 start, u32 end, string& error);

	optional<u32> FindNext(HE_TOKEN_TYPE type, u32 start, optional<u32> _end) 
	{
		u32 end;
		if (!_end.has_value()) 
		{
			end = tokens.size();
		}
		else 
		{
			end = _end.value();
		}
		he_assert(start <= end);
		he_assert(end <= tokens.size());
		for (u32 i = start; i < end; i++) 
		{
			if (tokens[i].type == type) 
			{
				return { i };
			}
		}
		return {};
	}

	optional<u32> FindNextMatchingParenthese(u32 start,optional<u32> _end,HE_TOKEN_TYPE parenthnese) 
	{
		u32 end = _end.has_value() ? _end.value() : tokens.size();
		he_assert(PeekExpect(parenthnese, start));
		HE_TOKEN_TYPE matching_token;
		switch (parenthnese) {
			case HE_TOKEN_LCURLY: matching_token = HE_TOKEN_RCURLY; break;
			case HE_TOKEN_LPARENTHESE: matching_token = HE_TOKEN_RPARENTHESE; break;
			//we should not reach this point
			default: he_assert(false);
		}

		u32 lps = 0, p = start;
		while (p < end) 
		{
			if (tokens[p].type == parenthnese) lps++;
			else if (tokens[p].type == matching_token) lps--;
			if (lps == 0) break;
			p++;
		}
		if (lps == 0) return p;
		else return {};
	}

	//num   ::= num
	//var   ::= id
	//paren ::= ( expr )
	//call  ::= id([expr[,expr]*])
	//prim  ::= num | var | paren | call
	optional<u32> FindNextPrimExprEnd(u32 start,u32 end) 
	{
		he_assert(start < tokens.size() && end < tokens.size());
		he_assert(tokens[start].type != HE_TOKEN_BINOP);

		if (PeekExpect(HE_TOKEN_LPARENTHESE, start)) 
		{
			auto v = FindNextMatchingParenthese(start,end, HE_TOKEN_LPARENTHESE);
			return v.has_value() ? optional<u32>{v.value() + 1} : nullopt;
		}
		else if (PeekExpect(HE_TOKEN_IDENTIFIER,start) && PeekExpect(HE_TOKEN_LPARENTHESE,start + 1)) 
		{
			auto v = FindNextMatchingParenthese(start + 1,end, HE_TOKEN_LPARENTHESE);
			return v.has_value() ? optional<u32>{v.value() + 1} : nullopt;
		}
		else 
		{
			u32 p;
			for (p = start; p < end;p++) {
				if (!PeekExpect(HE_TOKEN_IDENTIFIER,p) && !PeekExpect(HE_TOKEN_NUM,p)) {
					break;
				}
			}
			return p;
		}
	}

	optional<u32> FindExprEnd(u32 start,u32 end) 
	{
		u32 p = start;
		while (p < end) 
		{
			auto v = FindNextPrimExprEnd(p, end);
			if (!v.has_value()) return {};
			p = v.value();
			if (!ConsumeExpect(HE_TOKEN_BINOP, p,nullptr)) {
				break;
			}
		}
		return p;
	}

	u32			  FindNext(HE_TOKEN_TYPE type, u32 start, optional<u32> end, u32 default_value) 
	{
		if (auto v = FindNext(type, start, end);v.has_value()) 
		{
			return v.value();
		}
		else 
		{
			return default_value;
		}
	}
	bool		  PeekExpect(HE_TOKEN_TYPE type, u32 p) {
		return p < tokens.size() && tokens[p].type == type;
	}
	optional<string> ConsumeExpect(HE_TOKEN_TYPE type, u32& p,string* error) 
	{
		if (p >= tokens.size()) 
		{
			if (error != nullptr) *error = ErrorEOF(p, type);
			return {};
		}
		else if (tokens[p].type != type) 
		{
			if (error != nullptr) *error = ErrorMismatch(p, type);
			return {};
		}
		p = p + 1;
		return {tokens[p - 1].token};
	}
	void Consume(u32& p,string* val) {
		he_assert(p != tokens.size());
		if (val != nullptr) {
			*val = tokens[p].token;
		}
		p++;
	}

	string ErrorPrefix(u32 i) 
	{
		return "fail to parse ast error at (" + to_string(tokens[i].line) + ":" + to_string(tokens[i].start) + "-" + to_string(tokens[i].end) + ")";
	}

	string ErrorEOF(u32 i,HE_TOKEN_TYPE expect) 
	{
		return ErrorPrefix(i) + "expect a " + g_token_type_name_table[expect] + " but eof was found";
	}

	string ErrorMismatch(u32 i,HE_TOKEN_TYPE expect) 
	{
		return ErrorPrefix(i) + "expect a " + g_token_type_name_table[expect] + " but " + g_token_type_name_table[tokens[i].type] + " was found";
	}

	vector<Token> tokens;
};


#define EXPECT_AND_CONSUME_TOKEN(token,p,error) if(!ConsumeExpect(token,p,&error).has_value()) {\
return {}; \
}


optional<ptr<SignatureExpr>> ASTParser::ParseSignature(u32 start, u32& end, string& error) 
{
	u32 p = start;
	he_assert(ConsumeExpect(HE_TOKEN_FUNC, p, nullptr).has_value());
	string func_name;
	vector<Declearation> args;
	string rtype;
	if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error); v.has_value())
	{
		func_name = v.value();
	}
	else
	{
		return {};
	}

	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LPARENTHESE, p, error);

	if (PeekExpect(HE_TOKEN_IDENTIFIER, p))
	{
		while (true)
		{
			string type, arg;
			Consume(p, &type);
			if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error); v.has_value())
			{
				arg = v.value();
			}
			else
			{
				return {};
			}
			args.push_back({ type, arg });
			if (!PeekExpect(HE_TOKEN_COMMA, p))
			{
				break;
			}
			else
			{
				Consume(p, nullptr);
			}
		}
	}

	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_RPARENTHESE, p, error);

	if (PeekExpect(HE_TOKEN_RIGHT_ARROW, p))
	{
		Consume(p, nullptr);
		if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error); v.has_value())
		{
			rtype = v.value();
		}
		else
		{
			return {};
		}
	}
	else
	{
		rtype = "void";
	}
	end = p;
	
	return ptr<SignatureExpr>(new SignatureExpr(rtype, func_name, args, tokens[start]));
}

//func  ::= fn id([id id[,id id]*]) [-> id] {body}
optional<ptr<FuncExpr>> ASTParser::ParseFunc(u32 start,u32& end, string& error) 
{
	u32 p = start;
	ptr<SignatureExpr> signature;
	if (auto v = ParseSignature(p, p, error);v.has_value()) {
		signature = v.value();
	}
	else {
		return { };
	}

	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LCURLY, p, error);
	u32 body_end;
	if (auto v = FindNextMatchingParenthese(p - 1, {},HE_TOKEN_LCURLY); !v.has_value())
	{
		error = ErrorPrefix(p) + " no matching HE_TOKEN_RCURLY after a HE_TOKEN_LCURLY";
		return {};
	}
	else 
	{
		body_end = v.value();
	}
	end = body_end + 1;

	ptr<BodyExpr> body;
	if (auto v = ParseBody(p,body_end, error); v.has_value()) 
	{
		body = v.value();
		p = body_end;
	}
	else 
	{
		return {};
	}

	
	ptr<FuncExpr> expr = ptr<FuncExpr>(new FuncExpr(signature, body, tokens[start]));
	return expr;
}

//body  ::=  [expr[;expr|nil]*] 
optional<ptr<BodyExpr>> ASTParser::ParseBody(u32 start,u32 end, string& error) 
{
	if (start >= end) 
	{
		return ptr<BodyExpr>(new BodyExpr(vector<ptr<Expr>>{},nullptr,tokens[start]));
	}
	
	ptr<Expr> expr;
	vector<ptr<Expr>> body;
	u32 p = start;
	while(p < end) 
	{
		if (PeekExpect(HE_TOKEN_IF,p)) {
			u32 next_start = 0;
			if (auto v = ParseIf(p, next_start, error);v.has_value()) {
				p = next_start;
				if (expr != nullptr) body.push_back(expr);
				body.push_back(v.value());
				expr = nullptr;
				continue;
			}
			else {
				return {};
			}
		}
		u32 expr_end;
		expr_end = FindNext(HE_TOKEN_SEMICOLON, p, end, end);

		ptr<Expr> new_expr;
		if (expr_end != p) 
		{
			//is a declear expression
			if (PeekExpect(HE_TOKEN_MUT,p) || (PeekExpect(HE_TOKEN_IDENTIFIER,p) && PeekExpect(HE_TOKEN_IDENTIFIER,p + 1))) 
			{
				if (auto v = ParseDeclear(p, expr_end, error); v.has_value()) 
				{
					new_expr = v.value();
				}
				else {
					return {};
				}
			}
			//is a assign expression
			else if (PeekExpect(HE_TOKEN_IDENTIFIER,p) && PeekExpect(HE_TOKEN_ASSIGN,p+1))
			{
				if (auto v = ParseAssign(p, expr_end, error);v.has_value()) {
					new_expr = v.value();
				}
				else {
					return {};
				}
			}
			//treat it as a plain expression
			else if (auto v = ParseExpression(p, expr_end, error); v.has_value()) 
			{
				new_expr = v.value();
			}
			else 
			{
				return {};
			}
		}
		if (expr != nullptr) 
		{
			body.push_back(expr);
		}
		expr = new_expr;
		p = expr_end + 1;
	}

	if (PeekExpect(HE_TOKEN_SEMICOLON, end - 1)) {
		body.push_back(expr);
		expr = nullptr;
	}

	return ptr<BodyExpr>(new BodyExpr(body, expr, tokens[start]));
}

//expr  ::= prim [op prim]* 
optional<ptr<Expr>>     ASTParser::ParseExpression(u32 start, u32 end, string& error) 
{
	u32 p = start,prim_end = 0;
	if (auto v = FindNextPrimExprEnd(start, end);v.has_value()) 
	{
		prim_end = v.value();
	}
	else 
	{
		error = ErrorPrefix(start) + " invalid primary expression";
		return {};
	}
	ptr<Expr> lhs;
	if (auto v = ParsePrimExpression(p, prim_end, error); v.has_value()) 
	{
		lhs = v.value();
	}
	else 
	{
		return {};
	}
	
	if (prim_end == end) 
	{
		return lhs;
	}
	else 
	{
		return ParseBinExpression(prim_end, end, lhs, error);
	}
}


optional<ptr<DeclearExpr>>		ASTParser::ParseDeclear(u32 start, u32 end, string& error)
{
	u32 p = start;
	bool mut = false;
	if (PeekExpect(HE_TOKEN_MUT, p))
	{
		mut = true;
		Consume(p, nullptr);
	}

	string type, name;
	if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error);!v.has_value())
	{
		return {};
	}
	else 
	{
		type = v.value();
	}
	if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error); !v.has_value()) 
	{
		return {};
	}
	else 
	{
		name = v.value();
	}

	if (!PeekExpect(HE_TOKEN_ASSIGN,p))
	{
		if (end == p) 
			return ptr<DeclearExpr>(new DeclearExpr(mut, type, name, nullptr, tokens[start]));
		else 
		{
			error = ErrorPrefix(p - 1) + "expect a ; on the end of declearation";
			return {};
		}
	}

	Consume(p, nullptr);
	ptr<Expr> expr;
	if (auto v = ParseExpression(p, end, error);v.has_value()) 
	{
		expr = v.value();
	}
	else 
	{
		return {};
	}
	return ptr<DeclearExpr>(new DeclearExpr(mut, type, name, expr, tokens[start]));
}

optional<ptr<AssignExpr>>	ASTParser::ParseAssign(u32 start, u32 end, string& error) 
{
	u32 p = start;
	string name;
	if (auto v = ConsumeExpect(HE_TOKEN_IDENTIFIER, p, &error); !v.has_value())
	{
		return {};
	}
	else
	{
		name = v.value();
	}
	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_ASSIGN, p, error);
	ptr<Expr> expr;
	if (auto v = ParseExpression(p, end, error);v.has_value()) {
		expr = v.value();
	}
	else {
		return {};
	}
	return ptr<AssignExpr>(new AssignExpr(expr, name, tokens[start]));
}


optional<ptr<Expr>>	ASTParser::ParseBinExpression(u32 start, u32 end, ptr<Expr> lhs, string& error) {
	u32 p = start;
	he_assert(PeekExpect(HE_TOKEN_BINOP, p));
	while (p < end) {
		if (!PeekExpect(HE_TOKEN_BINOP, p)) {
			error = ErrorMismatch(p, HE_TOKEN_BINOP);
			return {};
		}
		u32 curr_prior;

		string op; 
		Consume(p, &op);
		curr_prior = g_operator_priority[op];

		u32 prim_end;
		if (auto v = FindNextPrimExprEnd( p,end);v.has_value()) {
			prim_end = v.value();
		}
		else 
		{
			prim_end = end;
		}
		
		ptr<Expr> rhs;
		if (auto v = ParsePrimExpression(p, prim_end, error); v.has_value()) 
		{
			rhs = v.value();
		}
		else 
		{
			return {};
		}
		p = prim_end;

		if (p != end) 
		{
			u32 next_prior;
			string next_op = tokens[p].token;
			next_prior = g_operator_priority[next_op];
			if (next_prior > curr_prior) 
			{
				if (auto v = ParseBinExpression(p, end, rhs, error);v.has_value()) 
				{
					rhs = v.value();
				}
				else 
				{
					return {};
				}
				return ptr<CalculateExpr>(new CalculateExpr(op, lhs, rhs,tokens[start]));
			}
		}
		lhs = ptr<CalculateExpr>(new CalculateExpr(op, lhs, rhs, tokens[start]));
	}
	
	return lhs;
}

//ifexpr ::= if ( expr ) { body } [elif ( expr ) {body} ]* [else {body} ]
optional<ptr<IfExpr>>   ASTParser::ParseIf(u32 start, u32& end, string& error)
{
	u32 p = start;
	he_assert(ConsumeExpect(HE_TOKEN_IF, p, nullptr));
	
	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LPARENTHESE, p, error);
	u32 cond_end;
	if (auto v = FindNextMatchingParenthese(p - 1, {}, HE_TOKEN_LPARENTHESE); !v.has_value())
	{
		error = ErrorPrefix(p) + "no matching right parenthese for '('";
		return {};
	}
	else 
	{
		cond_end = v.value();
	}

	ptr<Expr> cond;
	if (auto v = ParseExpression(p, cond_end, error);v.has_value()) 
	{
		cond = v.value();
	}
	else
	{
		return {};
	}
	p = cond_end + 1;

	EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LCURLY, p, error);
	u32 body_end;
	if (auto v = FindNextMatchingParenthese(p - 1, {}, HE_TOKEN_LCURLY); v.has_value())
	{
		body_end = v.value();
	}
	else 
	{
		error = ErrorPrefix(p - 1) + "no matching right curly brace '}'";
		return {};
	}

	ptr<BodyExpr> then_sect;
	if (auto v = ParseBody(p, body_end, error);v.has_value()) 
	{
		then_sect = v.value();
	}
	else 
	{
		return {};
	}

	p = body_end + 1;
	vector<ptr<BodyExpr>> elif_exprs;
	vector<ptr<Expr>> elif_cond_exprs;

	while (PeekExpect(HE_TOKEN_ELSEIF,p)) {
		Consume(p, nullptr);
		EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LPARENTHESE, p, error);

		u32 cond_end;
		if (auto v = FindNextMatchingParenthese(p - 1, {}, HE_TOKEN_LPARENTHESE); !v.has_value())
		{
			error = ErrorPrefix(p) + "no matching right parenthese for '('";
			return {};
		}
		else
		{
			cond_end = v.value();
		}

		ptr<Expr> cond;
		if (auto v = ParseExpression(p, cond_end, error); v.has_value())
		{
			cond = v.value();
		}
		else
		{
			return {};
		}
		p = cond_end + 1;
		elif_cond_exprs.push_back(cond);
		
		EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LCURLY, p, error);
		ptr<BodyExpr> elif_expr;
		u32 body_end;
		if (auto v = FindNextMatchingParenthese(p - 1, {}, HE_TOKEN_LCURLY); v.has_value())
		{
			body_end = v.value();
		}
		else
		{
			error = ErrorPrefix(p - 1) + "no matching right curly brace '}'";
			return {};
		}
		if (auto v = ParseBody(p, body_end, error); v.has_value())
		{
			elif_expr = v.value();
		}
		else
		{
			return {};
		}
		p = body_end + 1;
		elif_exprs.push_back(elif_expr);
	}

	ptr<BodyExpr> else_sect;
	if (PeekExpect(HE_TOKEN_ELSE,p)) 
	{
		Consume(p, nullptr);
		EXPECT_AND_CONSUME_TOKEN(HE_TOKEN_LCURLY, p, error);
		u32 body_end;
		if (auto v = FindNextMatchingParenthese(p - 1, {}, HE_TOKEN_LCURLY); v.has_value())
		{
			body_end = v.value();
		}
		else
		{
			error = ErrorPrefix(p - 1) + "no matching right curly brace '}'";
			return {};
		}
		if (auto v = ParseBody(p, body_end, error); v.has_value())
		{
			else_sect = v.value();
		}
		else
		{
			return {};
		}
		p = body_end + 1;
	}

	end = p;

	return ptr<IfExpr>(new IfExpr(cond, then_sect, else_sect,elif_exprs,elif_cond_exprs,tokens[start]));
}

//num   ::= num
//var   ::= id
//paren ::= ( expr )
//call  ::= id([expr[,expr]*])
//prim  ::= num | var | paren | call
optional<ptr<Expr>>	ASTParser::ParsePrimExpression(u32 start, u32 end, string& error) 
{
	if (start == end) 
	{
		error = ErrorPrefix(start) + " expect a primary expression but a " + g_token_type_name_table[tokens[start].type] + 
			" was found";
		return {};
	}
	//num or identifier
	if (end - start == 1) 
	{
		if (PeekExpect(HE_TOKEN_NUM,start)) 
		{
			string num;
			Consume(start, &num);
			return ptr<NumberExpr>(new NumberExpr(num, tokens[start]));
		}
		else if (PeekExpect(HE_TOKEN_IDENTIFIER,start)) 
		{
			string variable;
			Consume(start, &variable);
			return ptr<VariableExpr>(new VariableExpr(variable, tokens[start]));
		}
	}
	else 
	{
		if (PeekExpect(HE_TOKEN_LPARENTHESE,start)) 
		{
			assert(tokens[end - 1].type == HE_TOKEN_RPARENTHESE);
			if (auto v = ParseExpression(start + 1, end - 1, error);v.has_value()) 
			{
				return v.value();
			}
			else {
				return {};
			}
		}
		else if (PeekExpect(HE_TOKEN_IDENTIFIER,start)) 
		{
			if (end - start < 3 || tokens[end - 1].type != HE_TOKEN_RPARENTHESE 
				|| tokens[start + 1].type != HE_TOKEN_LPARENTHESE) 
			{
				error = ErrorPrefix(start) + " expect a call expression";
				return {};
			}
			u32 p = start;
			string name;
			vector<ptr<Expr>> args;
			Consume(p, &name);
			Consume(p, nullptr);
			while (p < end - 1) 
			{
				u32 expr_end;
				if (auto v = FindExprEnd(p,end))
				{
					expr_end = v.value();
				}
				else 
				{
					expr_end = end - 1;
				}

				if (auto v = ParseExpression(p, expr_end, error);!v.has_value())
				{
					return {};
				}
				else 
				{
					args.push_back(v.value());
				}
				p = expr_end + 1;
			}
			return ptr<CallExpr>(new CallExpr(name, args, tokens[start])) ;
		}
		else 
		{
			error = ErrorPrefix(start) + " expect a primary expression";
			return {};
		}
	}
}


optional<ptr<TopLevelExpr>> ASTParser::Parse(string& error) 
{
	vector<ptr<FuncExpr>> funcs;
	vector<ptr<SignatureExpr>> sigs;
	u32 p = 0,end = 0;
	while (p < tokens.size()) 
	{
		if (PeekExpect(HE_TOKEN_EXTERN,p)) {
			if (auto v = ParseExtern(p, end, error);v.has_value()) {
				sigs.push_back(v.value());
				p = end;
			}
			else {
				return {};
			}
			continue;
		}

		if (!PeekExpect(HE_TOKEN_FUNC,p))
		{
			error = ErrorMismatch(p, HE_TOKEN_FUNC);
			return {};
		}
		if (auto f = ParseFunc(p, end, error); f.has_value())
		{
			funcs.push_back(f.value());
		}
		else 
		{
			return {};
		}
		p = end;
	}
	return ptr<TopLevelExpr>(new TopLevelExpr(funcs, sigs, tokens[0]));
}

optional<ptr<SignatureExpr>> ASTParser::ParseExtern(u32 start, u32& end, string& error){
	u32 p = start;
	he_assert(ConsumeExpect(HE_TOKEN_EXTERN, p, nullptr));
	if (auto v = FindNext(HE_TOKEN_SEMICOLON, p, {});v.has_value()) {
		end = v.value();
	}
	else {
		error = ErrorPrefix(start) + " expected a ';' at end of can can need";
		return {};
	}
	auto rv = ParseSignature(p, end, error);
	end += 1;
	return rv;
}

bool AST::Parse(const vector<Token>& tokens) 
{
	error = "";
	ASTParser parser(tokens);
	if (auto v = parser.Parse(error);v.has_value()) {
		exprs = v.value();
	}
	else {
		return false;
	}
	return true;
}

string  AST::ErrorMsg() 
{
	return error;
}
