#pragma once
#include "common.h"
#include "config.h"

class IO {
private:
	static unique_ptr<IO> g_io;
	string m_search_path;
public:
	IO() {};
	static bool Initialize(Config& config);
	static IO&  Get();
	optional<string> LoadFile(const string& file);
};