#pragma once
#include "common.h"
#include "tokens.h"
#include "ast.h"
#include "cmdline.h"
#include "io.h"
#include "codegen.h"

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>

#include <filesystem>
namespace fs = std::filesystem;

bool Compile(const string& input,const string& output,Config& config) {
	ptr<LLVMCodeGenContext> context(new LLVMCodeGenContext(config));

	string target_triple = llvm::sys::getDefaultTargetTriple();
	context->llvm_module->setTargetTriple(target_triple);

	string code;
	if (auto v = IO::Get().LoadFile(input);v.has_value()) {
		code = v.value();
	}
	else {
		printf("helang: fail to load file %s",input.c_str());
		return false;
	}

	vector<Token> tokens;
	if (auto v = Lexer::Get().Parse(code);v.has_value()) {
		tokens = v.value();
	}
	else {
		printf("helang: %s",Lexer::Get().ErrorMsg().c_str());
		return false;
	}

	ptr<AST> ast(new AST);
	if (!ast->Parse(tokens)) {
		printf("helang: %s",ast->ErrorMsg().c_str());
		return false;
	}

	if (!context->GenerateCode(ast.get())) {
		printf("helang: %s",ast->ErrorMsg().c_str());
		return false;
	}

	std::string error;
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);
	if (target == nullptr) {
		printf("fail to compile file %s reason : %s\n", input.c_str(), error.c_str());
		return false;
	}

	llvm::TargetMachine* target_machine =
		target->createTargetMachine(target_triple, "generic", "", llvm::TargetOptions{}, {});
	context->llvm_module->setDataLayout(target_machine->createDataLayout());

	std::error_code EC;
	llvm::raw_fd_ostream dest(output, EC);
	if (EC) {
		llvm::errs() << "can't open file " << output << "\n";
		return false;
	}

	llvm::legacy::PassManager pass_manager;
	if (target_machine->addPassesToEmitFile(pass_manager,dest,nullptr,llvm::CGFT_ObjectFile)) {
		llvm::errs() << "llvm can't emit object file\n";
		return false;
	}

	pass_manager.run(*context->llvm_module);
	dest.flush();
	return true;
}

int main(int argc,const char** argvs) {
	Config config;
	auto dump_ir_callback = [&](ParamParser*, ParameterTable*, u32) {
		config.dump = true;
	};
	config.dump = false;

	ParameterTable paramTable[] = {
		ParameterTable("output","the output .o file",nullptr,nullptr,true,{"-O","-o","--ouptut"}),
		ParameterTable("input", "the input .he file",nullptr,nullptr,true,{"-C","-c","--compile"}),
		ParameterTable("help",  "print a helper message",nullptr,print_help_message,false,{"-H","-h","--help"}),
		ParameterTable("dump",  "print generated ir to stdio",nullptr,dump_ir_callback,false,{"-D","-d","--dump"}),
		ParameterTable("path",  "search path of the compiler",nullptr,nullptr,true,{"-P","-p","--path"}),
	};

	ParamParser parser(argc - 1, argvs + 1, he_countof(paramTable), paramTable);
	
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	string target = llvm::sys::getDefaultTargetTriple();

	vector<string> input_file, output_file;
	input_file = UnzipString(parser.Require<string>("input"));
	output_file = UnzipString(parser.Require<string>("output"));
	/*if (auto v = parser.Get<string>("output_file");v.has_value()) {
		output_file = UnzipString(v.value());
		if (input_file.size() != output_file.size()) {
			printf("the count of input files must equal to count of output files\n");
			return -1;
		}
	}
	else {
		for (u32 i = 0; i < input_file.size();i++) {
			output_file.push_back(input_file[i] + ".o");
		}
	}*/

	if (auto v = parser.Get<string>("path");!v.has_value()) {
		fs::path curr_path = argvs[0];
		config.search_path = curr_path.parent_path().string();
	}
	else {
		config.search_path = v.value();
	}

	if (!IO::Initialize(config)) {
		printf("fail to initialize io system");
		return -1;
	}
	Lexer::Initialize(config);
	
	for (u32 i = 0; i < input_file.size();i++) {
		if (!Compile(input_file[i],output_file[i],config)) {
			return -1;
		}
	}
	return 0;
}