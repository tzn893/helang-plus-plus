#pragma once
#include "common.h"
#include <functional>
#include <map>
//command line arguments parser


class ParamParser;

struct ParameterTable {
	const char* key;
	const char* discribtion;
	const char* default_value;
	function<void(ParamParser*,ParameterTable*,u32)> callback;
	bool  contains_value;
	vector<const char*> names;

	ParameterTable(const char* key,
	const char* discribtion,
	const char* default_value,
	function<void(ParamParser*, ParameterTable*, u32)> callback,
	bool  contains_value,
	vector<const char*> names):key(key),discribtion(discribtion),default_value(default_value),callback(callback),
	contains_value(contains_value),names(names) {}
};


void print_help_message(ParamParser*, ParameterTable* table, u32 count);

class ParamParser {
public:

	ParamParser(int argc,const char** argvs,u32 tableCount,ParameterTable* table);

	template<typename T>
	void Set(const string& key, const T& value) 
	{
		if constexpr (std::is_same_v<T, string>) 
		{
			return InternalSet(key, value);
		}
		else 
		{
			return InternalSet(key, std::to_string(value));
		}
	}

	template<typename T>
	optional<T> Get(const string& key) 
	{
		string value;
		if (auto v = InternalGet(key);v.has_value()) 
		{
			value = v.value();
		}
		else 
		{
			return {};
		}

		if constexpr (std::is_same_v<T, u32> || std::is_same_v<T, i32>) 
		{
			try {
				return stoi(value);
			}
			catch (...) {
				return {};
			}
		}
		else if constexpr(std::is_same_v<T,string>) 
		{
			return value;
		}
		else
		{
			return {};
		}
	}

	template<typename T>
	T Require(const string& key) {
		auto v = this->Get<T>(key);
		if (!v.has_value()) {
			printf("expect %s argument in command line\n",key.c_str());
			print_help_message(this, table_desc.data(), table_desc.size());
			exit(-1);
		}
		return v.value();
	}

private:
	void InternalSet(const string& key,const string& value);
	optional<string> InternalGet(const string& key);

	map<string, string> table;
	vector<ParameterTable> table_desc;
};


string ZipString(const vector<string>& strs);
vector<string> UnzipString(string& s);