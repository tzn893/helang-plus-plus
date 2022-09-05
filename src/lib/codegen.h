#pragma once
#include "ast.h"

//llvm contexts
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

struct LLVMCodeGenContext
{
	ptr<llvm::IRBuilder<>> ir_builder;
	ptr<llvm::LLVMContext> llvm_context;
	ptr<llvm::Module>      llvm_module;

	struct Context {
		unordered_map<string, llvm::Value*> ssa;
		unordered_map<string, llvm::AllocaInst*> variable;
		string func_name;
	};
	vector<Context> context;
	string error;
	bool   dump;

	LLVMCodeGenContext(Config& config);

	optional<llvm::Value*> FindSSAValue(const string& name);

	bool InsertSSA(const string& name, llvm::Value* value);

	optional<llvm::AllocaInst*> FindVariable(const string& name);

	bool InsertVariable(const string& name, const string& type, string& error);

	void PopContext();

	void PushContext(const string& func_name);

	//currently only int type is supported
	optional<llvm::Type*> CreateLLVMType(string type);
	//return nullptr if the type is void
	//return a value if the type has a default value
	optional<llvm::Value*> CreateLLVMTypeDefaultValue(string type);

	bool GenerateCode(AST* ast);

	string ErrorMsg() { return error; }

	llvm::Function* GetContextFunction();
};