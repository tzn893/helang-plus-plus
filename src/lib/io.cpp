#include "io.h"
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

unique_ptr<IO> IO::g_io;

bool IO::Initialize(Config& config) {
	fs::path p(config.search_path);
	if (!fs::exists(config.search_path)) {
		return false;
	}
	if (!fs::is_directory(p)) {
		p = p.parent_path();
	}
	g_io = make_unique<IO>();
	g_io->m_search_path = p.string();
	if (!fs::is_directory(g_io->m_search_path)) {
		g_io->m_search_path = fs::path(g_io->m_search_path).parent_path().string();
	}
	return true;
}

IO& IO::Get() {
	he_assert(g_io != nullptr);
	return *g_io.get();
}

optional<string> IO::LoadFile(const string& file) {
	fs::path path = file;
	
	if (!fs::exists(path)) {
		if (path = m_search_path / path;!fs::exists(path)) {
			return {};
		}
	}

	ifstream fstream(path.string(), std::ios::binary | std::ios::ate);
	u32 fsize = fstream.tellg();
	vector<char> buffer(fsize + 1);
	fstream.seekg(0,std::ios::beg);
	
	if (fstream.read(buffer.data(),fsize)) {
		buffer[fsize] = '\0';
		return { string(buffer.data()) };
	}
	return {};
}