#include <filesystem>
#include <Windows.h>
#include "cmdline.h"
namespace fs = std::filesystem;
using namespace std;

int main(int argn,const char** argvs) {
	bool dump = false;
	auto dump_call_back = [&](ParamParser*, ParameterTable* table, u32 count) {
		dump = true;
	};
	ParameterTable paramTable[] = {
		ParameterTable("input", "the input .he file",nullptr,nullptr,true,{"-C","-c","--compile"}),
		ParameterTable("output", "the output .he file",nullptr,nullptr,true,{"-O","-o","--output"}),
		ParameterTable("help",  "print a helper message",nullptr,print_help_message,false,{"-H","-h","--help"}),
		ParameterTable("dump",  "print generated ir to stdio",nullptr,dump_call_back,false,{"-D","-d","--dump"})
	};
	ParamParser parser(argn - 1, argvs + 1, he_countof(paramTable), paramTable);

	fs::path p(argvs[0]);
	p = p.parent_path();

	string clang_cmd = (p / "clang").string();
	string helang_c_cmd = (p / "helang-c").string();

	string exe = parser.Require<string>("output");
	vector<string> inputs = UnzipString(parser.Require<string>("input"));
	vector<string> outputs;
	for (auto& s : inputs) {
		outputs.push_back(s + ".o");
	}
	helang_c_cmd += " -c";
	for (auto i : inputs) {
		helang_c_cmd += " " + i;
	}
	helang_c_cmd += " -o";
	for (auto o : outputs) {
		helang_c_cmd += " " + o;
	}
	if (dump) {
		helang_c_cmd += " -d";
	}
	char buffer[65536];
	memcpy(buffer, helang_c_cmd.c_str(), helang_c_cmd.size());
	buffer[helang_c_cmd.size()] = '\0';

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcessA(
		NULL,   //  指向一个NULL结尾的、用来指定可执行模块的宽字节字符串  
		buffer, // 命令行字符串  
		NULL, //    指向一个SECURITY_ATTRIBUTES结构体，这个结构体决定是否返回的句柄可以被子进程继承。  
		NULL, //    如果lpProcessAttributes参数为空（NULL），那么句柄不能被继承。<同上>  
		false,//    指示新进程是否从调用进程处继承了句柄。   
		0,  //  指定附加的、用来控制优先类和进程的创建的标  
			//  CREATE_NEW_CONSOLE  新控制台打开子进程  
			//  CREATE_SUSPENDED    子进程创建后挂起，直到调用ResumeThread函数  
		NULL, //    指向一个新进程的环境块。如果此参数为空，新进程使用调用进程的环境  
		NULL, //    指定子进程的工作路径  
		&si, // 决定新进程的主窗体如何显示的STARTUPINFO结构体  
		&pi  // 接收新进程的识别信息的PROCESS_INFORMATION结构体  
	)) {
		printf("fail to create process helang-c\n");
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD value = 0;
	GetExitCodeProcess(pi.hProcess, &value);
	if (value != 0) {
		for (auto& o : outputs) {
			DeleteFileA(o.c_str());
		}
		printf("fail to compile helang\n");
		return 1;
	}

	for (auto o : outputs) {
		clang_cmd += " " + o;
	}
	clang_cmd += " " + (p / "io.c").string() + " " + (p / "main.c").string();

	
	clang_cmd += " -o " + exe;

	memcpy(buffer, clang_cmd.c_str(), clang_cmd.size());
	buffer[clang_cmd.size()] = '\0';

	if (!CreateProcessA(
		NULL,   //  指向一个NULL结尾的、用来指定可执行模块的宽字节字符串  
		buffer, // 命令行字符串  
		NULL, //    指向一个SECURITY_ATTRIBUTES结构体，这个结构体决定是否返回的句柄可以被子进程继承。  
		NULL, //    如果lpProcessAttributes参数为空（NULL），那么句柄不能被继承。<同上>  
		false,//    指示新进程是否从调用进程处继承了句柄。   
		0,  //  指定附加的、用来控制优先类和进程的创建的标  
			//  CREATE_NEW_CONSOLE  新控制台打开子进程  
			//  CREATE_SUSPENDED    子进程创建后挂起，直到调用ResumeThread函数  
		NULL, //    指向一个新进程的环境块。如果此参数为空，新进程使用调用进程的环境  
		NULL, //    指定子进程的工作路径  
		&si, // 决定新进程的主窗体如何显示的STARTUPINFO结构体  
		&pi  // 接收新进程的识别信息的PROCESS_INFORMATION结构体
	)) {
		for (auto& o : outputs) {
			DeleteFileA(o.c_str());
		}
		printf("fail to create process clang\n");
		return 1;
	}
	
	WaitForSingleObject(pi.hProcess, INFINITE);
	for (auto& o : outputs) {
		DeleteFileA(o.c_str());
	}
	return 0;
}