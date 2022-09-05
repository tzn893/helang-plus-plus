#pragma once
#include "common.h"
#include "config.h"

enum HE_TOKEN_TYPE {
	HE_TOKEN_NUM = 0,//[0-9]+
	HE_TOKEN_IDENTIFIER,//[a-zA-Z][a-zA-Z0-9]*
	HE_TOKEN_SEMICOLON,//;
	HE_TOKEN_LPARENTHESE,//(
	HE_TOKEN_RPARENTHESE,//)
	HE_TOKEN_LCURLY,//{
	HE_TOKEN_RCURLY,//}
	HE_TOKEN_BINOP,//+-*/|
	HE_TOKEN_RIGHT_ARROW,//->
	HE_TOKEN_ASSIGN,//=
	HE_TOKEN_MUT,//mut
	HE_TOKEN_FUNC,//fn
	HE_TOKEN_COMMA,//,
	HE_TOKEN_IF,//if
	HE_TOKEN_ELSE,//else,
	HE_TOKEN_ELSEIF,//elif
	HE_TOKEN_EXTERN,//ccnd
	HE_TOKEN_COUNT
};


struct Token {
	HE_TOKEN_TYPE type;
	string        token;
	//for debuging
	u32			  line;
	u32			  start,end;
	string ToString();
};

class Lexer {
private:
	static unique_ptr<Lexer> m_lexer;
	string error;
public:
	Lexer() { }
	static void Initialize(Config& config);
	static Lexer& Get();
	optional<vector<Token>> Parse(const string& content);
	string ErrorMsg() { return error; }
};

