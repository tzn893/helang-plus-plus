#include "tokens.h"
#include <unordered_map>
#include <sstream>
#include <tuple>
unique_ptr<Lexer> Lexer::m_lexer;
static string g_entry;
unordered_map<string, Token> m_keyword_map;

const char* g_token_type_name_table[HE_TOKEN_COUNT];
unordered_map<string, u32> g_operator_priority;

#define FILL_TOKEN_NAME_TABLE(t) g_token_type_name_table[t] = ""#t;

class _GlobalSettingInitializer {
public:
	_GlobalSettingInitializer() {
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_NUM);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_IDENTIFIER);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_SEMICOLON);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_LPARENTHESE);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_RPARENTHESE);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_LCURLY);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_RCURLY);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_BINOP);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_RIGHT_ARROW);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_FUNC);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_COMMA);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_ASSIGN);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_IF);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_ELSE);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_ELSEIF);
		FILL_TOKEN_NAME_TABLE(HE_TOKEN_EXTERN);

		g_operator_priority["+"] = 10;
		g_operator_priority["-"] = 20;
		g_operator_priority["*"] = 30;
		g_operator_priority["/"] = 40;
		g_operator_priority["=="] = 9;
		g_operator_priority["!="] = 8;
		g_operator_priority["|"] = 6;
	}
};

static _GlobalSettingInitializer _settingInitializer;

void Lexer::Initialize(Config& config) {
	m_keyword_map["fn"] = { HE_TOKEN_FUNC,"fn" };
	m_keyword_map["mut"] = { HE_TOKEN_MUT,"mut" };
	m_keyword_map["if"] = { HE_TOKEN_IF,"if" };
	m_keyword_map["else"] = { HE_TOKEN_ELSE,"else" };
	m_keyword_map["elif"] = {HE_TOKEN_ELSEIF,"elif"};
	m_keyword_map["ccnd"] = { HE_TOKEN_EXTERN,"ccnd" };
	m_lexer = make_unique<Lexer>();
}


Lexer& Lexer::Get() {
	he_assert(m_lexer != nullptr);
	return *m_lexer.get();
}

static string lexerErrorPrefix(string code, u32 line, u32 p) {
	return "lexer : error at (" + to_string(line) + "," + to_string(p) + "):";
}

optional<tuple<u32, Token>> alphaParser(u32, const string&, string&);
optional<tuple<u32, Token>> numericParser(u32,const string&,string&);
optional<tuple<u32, Token>> signParser(u32, const string&, string&);

optional<vector<Token>> Lexer::Parse(const string& content) {
	istringstream ss(content);
	string content_line;

	vector<Token> tokens;
	u32 line_count = 0;
	while (getline(ss, content_line)) {
		line_count++;
		u32 p = 0;
		while (p < content_line.size()) {
			if (content_line[p] == ' ' || content_line[p] == '\t' || content_line[p] == '\r') { p++; continue; }
			if (content_line[p] == '\n') break;

			optional<tuple<u32, Token>> res;
			if (isalpha(content_line[p]) || content_line[p] == '_') {
				res = alphaParser(p, content_line, error);
			}
			else if (isdigit(content_line[p])) {
				res = numericParser(p, content_line, error);
			}else if (content_line[p] == '#') {
				//skip comments
				break;
			}
			else{
				res = signParser(p, content_line, error);
			}

			if (res.has_value()) {
				auto [np, token] = res.value();
				token.line = line_count;
				token.start = p,token.end = np;
				tokens.push_back(token);
				p = np;
			}
			else {
				error = lexerErrorPrefix(content_line, line_count, p) + error;
				return {};
			}
		}
	}
	return { std::move(tokens) };
}

optional<tuple<u32, Token>> alphaParser(u32 p, const string& str, string& error) {
	he_assert(p < str.size() && isalpha(str[p]));
	u32 e = p;
	while (e != str.size() && (isalpha(str[e]) || isdigit(str[e]) || str[e] == '_')) e++;
	string token = str.substr(p, e - p);
	if (auto res = m_keyword_map.find(token);res != m_keyword_map.end()) {
		return { make_tuple(e,res->second) };
	}
	return { make_tuple(e,Token{HE_TOKEN_IDENTIFIER,token}) };
}

optional<tuple<u32, Token>> numericParser(u32 p, const string& str, string& error) {
	he_assert(p < str.size() && isdigit(str[p]));
	u32 e = p;
	while (e != str.size() && isdigit(str[e]) ) e++;
	string token = str.substr(p, e - p);
	return { make_tuple(e,Token{HE_TOKEN_NUM,token}) };
}

optional<tuple<u32, Token>> signParser(u32 p, const string& str, string& error) {
	he_assert(p < str.size());
	switch (str[p]) {
		case ';': return  make_tuple(p + 1, Token{ HE_TOKEN_SEMICOLON,";" });
		case '(': return  make_tuple(p + 1, Token{ HE_TOKEN_LPARENTHESE,"(" });
		case ')': return  make_tuple(p + 1, Token{ HE_TOKEN_RPARENTHESE,")" });
		case '{': return  make_tuple(p + 1, Token{ HE_TOKEN_LCURLY,"{" });
		case '}': return  make_tuple(p + 1, Token{ HE_TOKEN_RCURLY,"}" });
		case '+': return  make_tuple(p + 1, Token{ HE_TOKEN_BINOP,"+" });
		case '-': {
			if (p + 1 < str.size() && str[p + 1] == '>') {
				return make_tuple(p + 2, Token{ HE_TOKEN_RIGHT_ARROW,"->"});
			}
			return  make_tuple(p + 1, Token{ HE_TOKEN_BINOP,"-" });
		}
		case '*': return  make_tuple(p + 1, Token{ HE_TOKEN_BINOP,"*" });
		case '/': return  make_tuple(p + 1, Token{ HE_TOKEN_BINOP,"/" });
		case '.': return  make_tuple(p + 1, Token{ HE_TOKEN_BINOP,"." });
		case ',': return  make_tuple(p + 1, Token{ HE_TOKEN_COMMA,"," });
		case '=': {
			if (p + 1 < str.size() && str[p + 1] == '=') {
				return make_tuple(p + 2, Token{ HE_TOKEN_BINOP,"==" });
			}
			return  make_tuple(p + 1, Token{ HE_TOKEN_ASSIGN,"=" });
		}
		case '!': {
			if (p + 1 < str.size() && str[p + 1] == '=') {
				return make_tuple(p + 2, Token{ HE_TOKEN_BINOP,"!=" });
			}
		}
		case '|': return make_tuple(p + 1, Token{ HE_TOKEN_BINOP, "|" });
	}

	error = string("unrecognized character \"") + str[p] + " \"";
	return {};
}


string Token::ToString() {
	string blank = string(20 - strlen(g_token_type_name_table[type]),' ');
	return "type:" + string(g_token_type_name_table[type]) + blank + "value:{" + token + "}\tat (" +
		to_string(line) + ":" + to_string(start)+"-"+to_string(end)+")";
}

