#include "cmdline.h"
#include <filesystem>
#include <sstream>

void ParamParser::InternalSet(const string& key, const string& value) {
	table[key] = value;
}

optional<string> ParamParser::InternalGet(const string& key) {
	if (auto res = table.find(key);res != table.end()) {
		return { res->second };
	}
	return {};
}

void print_help_message(ParamParser*, ParameterTable* table, u32 count) {
	printf("options : (* stands for options required)\n");
	for (u32 i = 0; i < count; i++) {
		printf("%-10s\t%-30s\t\n                values:", table[i].key, table[i].discribtion);
		for (u32 j = 0; j < table[i].names.size(); j++) {
			printf("%s ", table[i].names[j]);
		}
		printf("\n");
	}
	exit(0);
}

ParamParser::ParamParser(int argc, const char** argvs, u32 tableCount, ParameterTable* table) {
	table_desc.insert(table_desc.begin(), table, table + tableCount);
	map<string, ParameterTable*> parameterTable;
	for (u32 i = 0; i < tableCount;i++) {
		for (u32 j = 0; j < table[i].names.size(); j++) {
			parameterTable[table[i].names[j]]
				= table + i;
		}
		if (table[i].default_value) {
			this->table[table[i].key] = table[i].default_value;
		}
	}
	for (u32 i = 0; i < argc;) {
		string argv = argvs[i];
		if (auto res = parameterTable.find(argv);res != parameterTable.end()) {
			ParameterTable* t = res->second;
			if (i + 1 < argc && t->contains_value) {
				vector<string> value;
				u32 j = i + 1;
				for (;j < argc; j++) {
					if (argvs[j][0] != '-')
						value.push_back(argvs[j]);
					else break;
				}
				this->table[t->key] = ZipString(value);
				i = j;
			}
			else {
				i++;
			}
			if (t->callback != nullptr) {
				t->callback(this, table, tableCount);
			}
		}
		else {
			printf("error: unrecognized option %s usage:\n", argvs[i]);
			print_help_message(nullptr,table,tableCount);
			exit(-1);
		}
	}
}

string ZipString(const vector<string>& strs) {
	string res;
	for (u32 i = 0; i < strs.size();i++) {
		if (i != 0) res += ';';
		res += strs[i];
	}
	return res;
}

vector<string> UnzipString(string& s) {
	vector<string> res;
	for (u32 i = 0; i < s.size();) {
		i32 j = s.find_first_of(';',i);
		if (j < 0) j = s.size();
		res.push_back(s.substr(i,j - i));
		i = j + 1;
	}
	return res;
}