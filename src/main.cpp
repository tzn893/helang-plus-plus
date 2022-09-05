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
		NULL,   //  ָ��һ��NULL��β�ġ�����ָ����ִ��ģ��Ŀ��ֽ��ַ���  
		buffer, // �������ַ���  
		NULL, //    ָ��һ��SECURITY_ATTRIBUTES�ṹ�壬����ṹ������Ƿ񷵻صľ�����Ա��ӽ��̼̳С�  
		NULL, //    ���lpProcessAttributes����Ϊ�գ�NULL������ô������ܱ��̳С�<ͬ��>  
		false,//    ָʾ�½����Ƿ�ӵ��ý��̴��̳��˾����   
		0,  //  ָ�����ӵġ���������������ͽ��̵Ĵ����ı�  
			//  CREATE_NEW_CONSOLE  �¿���̨���ӽ���  
			//  CREATE_SUSPENDED    �ӽ��̴��������ֱ������ResumeThread����  
		NULL, //    ָ��һ���½��̵Ļ����顣����˲���Ϊ�գ��½���ʹ�õ��ý��̵Ļ���  
		NULL, //    ָ���ӽ��̵Ĺ���·��  
		&si, // �����½��̵������������ʾ��STARTUPINFO�ṹ��  
		&pi  // �����½��̵�ʶ����Ϣ��PROCESS_INFORMATION�ṹ��  
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
		NULL,   //  ָ��һ��NULL��β�ġ�����ָ����ִ��ģ��Ŀ��ֽ��ַ���  
		buffer, // �������ַ���  
		NULL, //    ָ��һ��SECURITY_ATTRIBUTES�ṹ�壬����ṹ������Ƿ񷵻صľ�����Ա��ӽ��̼̳С�  
		NULL, //    ���lpProcessAttributes����Ϊ�գ�NULL������ô������ܱ��̳С�<ͬ��>  
		false,//    ָʾ�½����Ƿ�ӵ��ý��̴��̳��˾����   
		0,  //  ָ�����ӵġ���������������ͽ��̵Ĵ����ı�  
			//  CREATE_NEW_CONSOLE  �¿���̨���ӽ���  
			//  CREATE_SUSPENDED    �ӽ��̴��������ֱ������ResumeThread����  
		NULL, //    ָ��һ���½��̵Ļ����顣����˲���Ϊ�գ��½���ʹ�õ��ý��̵Ļ���  
		NULL, //    ָ���ӽ��̵Ĺ���·��  
		&si, // �����½��̵������������ʾ��STARTUPINFO�ṹ��  
		&pi  // �����½��̵�ʶ����Ϣ��PROCESS_INFORMATION�ṹ��
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