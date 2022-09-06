#include "ast.h"
#include "codegen.h"
#include <sstream>

using namespace llvm;

LLVMCodeGenContext::LLVMCodeGenContext(Config& config)
{
	dump = config.dump;

	llvm_context = ptr<llvm::LLVMContext>(new llvm::LLVMContext());
	llvm_module = ptr<llvm::Module>(new llvm::Module("helang", *llvm_context));
	ir_builder = ptr<llvm::IRBuilder<>>(new llvm::IRBuilder<>(*llvm_context));

	//global context
	context.push_back(Context{ {},{},"__global"});
}

Function* LLVMCodeGenContext::GetContextFunction() {
	return llvm_module->getFunction(context.back().func_name);
}

optional<llvm::Value*> LLVMCodeGenContext::FindSSAValue(const string& name)
{
	auto& ssa_context = context.back().ssa;
	//TODO find outer layers
	if (auto v = ssa_context.find(name);
		v != ssa_context.end())
	{
		return v->second;
	}
	else
	{
		return {};
	}
}

bool LLVMCodeGenContext::InsertSSA(const string& name, llvm::Value* value)
{
	auto& variable_context = context.back().variable;
	auto& ssa_context = context.back().ssa;

	if (variable_context.count(name) || ssa_context.count(name))
	{
		return false;
	}
	ssa_context[name] = value;
	return true;
}

optional<llvm::AllocaInst*> LLVMCodeGenContext::FindVariable(const string& name) {
	auto& variable_context = context.back().variable;
	if (auto v = variable_context.find(name);
		v != variable_context.end())
	{
		return v->second;
	}
	else
	{
		return {};
	}
}

bool LLVMCodeGenContext::InsertVariable(const string& name, const string& type, string& error)
{
	auto& variable_context = context.back().variable;
	auto& ssa_context = context.back().ssa;

	if (variable_context.count(name) || ssa_context.count(name))
	{
		error = "fail to create variable " + name + " variable has exists under this context";
		return false;
	}
	string func_name = context.back().func_name;
	llvm::Function* func = llvm_module->getFunction(func_name);
	he_assert(func != nullptr);
	llvm::Type* var_type;

	if (auto t = CreateLLVMType(type); t.has_value())
	{
		var_type = t.value();
	}
	else
	{
		error = "undefined type " + type;
		return {};
	}

	llvm::IRBuilder<> tmp(&func->getEntryBlock(), func->getEntryBlock().begin());
	llvm::AllocaInst* alloc = tmp.CreateAlloca(var_type, nullptr, name);
	he_assert(alloc != nullptr);

	context.back().variable[name] = alloc;
	return true;
}

void LLVMCodeGenContext::PopContext()
{
	context.pop_back();
	he_assert(context.size() >= 1);
}

void LLVMCodeGenContext::PushContext(const string& func_name)
{
	Context c;
	c.func_name = func_name;
	context.push_back(c);
}

//currently only int type is supported
optional<llvm::Type*> LLVMCodeGenContext::CreateLLVMType(string type) {
	if (type == "i32") {
		return llvm::Type::getInt32Ty(*llvm_context);
	}
	else if (type == "u8") {
		return llvm::Type::getInt64Ty(*llvm_context);
	}
	else {
		return {};
	}
}

optional<llvm::Value*> LLVMCodeGenContext::CreateLLVMTypeDefaultValue(string type){
	if (type == "void") {
		return { nullptr };
	}
	else if(type == "i32") {
		return { llvm::ConstantInt::get(*llvm_context,llvm::APInt(32,0)) };
	}
	else {
		return {};
	}
}

static LLVMCodeGenContext* g_context;
constexpr const char* entry_prefix = "__he_entry_";
constexpr const char* entry = "main";

bool LLVMCodeGenContext::GenerateCode(AST* ast) {
	g_context = this;
	if (auto v = ast->GenerateIRCode();v.has_value()) {
		if (dump) 
			printf("generated code %s",v.value().c_str());
	}
	else {
		error = ast->ErrorMsg();
		return false;
	}
}


string Expr::ErrorPrefix() 
{
	return "fail to generate IR code at function " + g_context->context.back().func_name
		+ "(" + to_string(line) + ":" + to_string(start) + "-" + to_string(end) + ")";
}

optional<llvm::Value*> NumberExpr::CodeGenerate(string& error) 
{
	u32 number;
	try 
	{
		number = stoi(num);
	}
	catch (...) 
	{
		error = ErrorPrefix() + " fail to cast " + num + " to integrate";
		return {};
	}
	return ConstantInt::get(*g_context->llvm_context, APInt(32,number));
}


optional<llvm::Value*> VariableExpr::CodeGenerate(string& error) 
{
	if (auto v = g_context->FindSSAValue(name); v.has_value())
	{
		return v.value();
	}
	else if (auto v = g_context->FindVariable(name);v.has_value()) 
	{
		return g_context->ir_builder->CreateLoad(v.value()->getAllocatedType(), v.value(),name);
	}
	else 
	{
		error = ErrorPrefix() + " undefined variable " + name;
		return {};
	}
}


optional<Value*> CalculateExpr::CodeGenerate(string& error) 
{
	llvm::Value* lhs_value,* rhs_value;
	if (auto v = lhs->CodeGenerate(error); v.has_value()) 
	{
		lhs_value = v.value();
	}
	else 
	{
		return {};
	}
	if (auto v = rhs->CodeGenerate(error); v.has_value()) 
	{
		rhs_value = v.value();
	}
	else 
	{
		return {};
	}

	//TODO : complex type system
	if (op == "+") 
	{
		return g_context->ir_builder->CreateAdd(lhs_value, rhs_value);
	}
	else if (op == "-") 
	{
		return g_context->ir_builder->CreateSub(lhs_value, rhs_value);
	}
	else if (op == "*")
	{
		return  g_context->ir_builder->CreateMul(lhs_value, rhs_value);
	}
	else if (op == "/")
	{
		return g_context->ir_builder->CreateUDiv(lhs_value, rhs_value);
	}
	else if (op == "==") {
		return g_context->ir_builder->CreateICmpEQ(lhs_value, rhs_value);
	}
	else if (op == "!=") {
		return g_context->ir_builder->CreateICmpNE(lhs_value, rhs_value);
	}
	else if (op == "|") {
		Type* t64 = llvm::Type::getInt64Ty(*g_context->llvm_context);
		return g_context->ir_builder->CreateOr(
			g_context->ir_builder->CreateIntCast(g_context->ir_builder->CreateShl(lhs_value, 8), t64,false),
			g_context->ir_builder->CreateIntCast(rhs_value,t64,false)
		);
	}
	else 
	{
		assert(false);
		return {};
	}
}


optional<llvm::Value*> CallExpr::CodeGenerate(string& error) 
{
	Function* func = g_context->llvm_module->getFunction(this->func.c_str());
	if (func == nullptr) 
	{
		error = ErrorPrefix() + " invalid function call, function \'" + this->func + "\''s definition is not found";
		return {};
	}

	if (args.size() != func->arg_size()) 
	{
		error = ErrorPrefix() + " function call expect " + to_string(func->arg_size()) + " arguments but " + to_string(args.size()) + " was found";
		return {};
	}
	
	vector<Value*> argvs;
	for (auto& arg : args) 
	{
		llvm::Value* argv;
		if (auto v = arg->CodeGenerate(error); v.has_value())
		{
			argv = v.value();
		}
		else return {};
		argvs.push_back(argv);
	}

	return g_context->ir_builder->CreateCall(func, argvs);
}

//this function should not be called,we should call function generate instead
optional<llvm::Value*> FuncExpr::CodeGenerate(string& error)
{
	//function expression will generate a Function* rather than value
	he_assert(false);
	return {};
}


optional<llvm::Function*> SignatureExpr::FunctionSignatureGenerate(string& error) {
	if (name == entry)
	{
		name = entry_prefix + name;
	}
	vector<Type*> argts; Type* rt_type = Type::getVoidTy(*g_context->llvm_context);
	for (auto arg_decl : args)
	{
		if (auto v = g_context->CreateLLVMType(arg_decl.type); v.has_value())
		{
			argts.push_back(v.value());
		}
		else
		{
			error = ErrorPrefix() + " undefined argument type " + arg_decl.type;
			return {};
		}
	}

	if (return_type != "void")
	{
		if (auto v = g_context->CreateLLVMType(return_type); v.has_value())
		{
			rt_type = v.value();
		}
		else
		{
			error = ErrorPrefix() + " undefined return type " + return_type;
			return {};
		}
	}

	FunctionType* ftype = FunctionType::get(rt_type, argts, false);
	Function* func = Function::Create(ftype, Function::ExternalLinkage, name, *g_context->llvm_module);

	u32 idx = 0;
	for (auto& arg : func->args())
	{
		arg.setName(args[idx++].name);
	}
	return func;
}

optional<llvm::Value*> SignatureExpr::CodeGenerate(string& error) {
	he_assert(false);
	return {};
}

optional<llvm::Function*> FuncExpr::FunctionSignatureGenerate(string& error) 
{
	return signature->FunctionSignatureGenerate(error);
}

bool BodyExpr::GenerateBodyCode(string& error,bool generate_return) {
	Function* func = g_context->GetContextFunction();
	he_assert(func != nullptr);

	for (auto& expr : body)
	{
		if (!expr->CodeGenerate(error).has_value())
		{
			return false;
		}
	}

	if (rt_expr != nullptr)
	{
		if (auto v = rt_expr->CodeGenerate(error); v.has_value())
		{
			if(generate_return) 
				g_context->ir_builder->CreateRet(v.value());
		}
		else
		{
			return false;
		}
	}

	return true;
}


optional<llvm::Value*> BodyExpr::CodeGenerate(string& error) {
	//this function should not be called
	he_assert(false);
	return { nullptr };
}

bool FuncExpr::FunctionBodyGenerate(string& error) 
{
	g_context->PushContext(signature->GetName());
	Function* func = g_context->GetContextFunction();

	string func_name = func->getName().data();
	BasicBlock* BB = BasicBlock::Create(*g_context->llvm_context, "body", func);
	g_context->ir_builder->SetInsertPoint(BB);

	for (auto& arg : func->args())
	{
		if (!g_context->InsertSSA(string(arg.getName()), &arg))
		{
			error = ErrorPrefix() + " repeated function argument " + string(arg.getName());
			return false;
		}
	}


	bool v = body->GenerateBodyCode(error,true);

	if (!v) {
		func->eraseFromParent();
		return false;
	}
	if (!body->HasReturnValue()) 
	{
		if (auto v = g_context->CreateLLVMTypeDefaultValue(signature->GetReturnType());!v.has_value()) 
		{
			error = ErrorPrefix() + " function's return type " + signature->GetReturnType() +
				" doesn't have a default value so a return value must be manually specified";
			return false;
		}
		else 
		{
			if (v.value() == nullptr) 
			{
				g_context->ir_builder->CreateRetVoid();
			}
			else 
			{
				g_context->ir_builder->CreateRet(v.value());
			}
		}
	}
	
	raw_string_ostream ss(error);
	if (verifyFunction(*func, &ss))
	{
		func->eraseFromParent();
		error = ErrorPrefix() + error;
		return false;
	}
	g_context->PopContext();
	
	return true;
}

optional<llvm::Value*> TopLevelExpr::CodeGenerate(string& error) 
{
	//you should not reach this point
	he_assert(false);
	return {};
}

optional<string> TopLevelExpr::IRGenerate(string& error) {
	vector<Function*> gen_funcs;
	for (auto f : funcs) {
		if (auto v = f->FunctionSignatureGenerate(error);!v.has_value()) {
			return {};
		}
		else {
			gen_funcs.push_back(v.value());
		}
	}
	for (auto f : extern_funcs) {
		if (auto v = f->FunctionSignatureGenerate(error);!v.has_value()) {
			return {};
		}
	}
	string output;
	raw_string_ostream ss(output);
	for (u32 i = 0; i < gen_funcs.size();i++) {
		if (!funcs[i]->FunctionBodyGenerate(error)) {
			return {};
		}
		gen_funcs[i]->print(ss);
	}
	return { output };
}

optional<string> AST::GenerateIRCode() {
	if (exprs == nullptr) {
		error = "empty ast";
		return {};
	}
	return exprs->IRGenerate(error);
}

optional<llvm::Value*> AssignExpr::CodeGenerate(string& error) 
{
	Value* expr_val;
	if (auto v = expr->CodeGenerate(error); v.has_value())
	{
		expr_val = v.value();
	}
	else 
	{
		return {};
	}


	AllocaInst* var;
	if (auto v = g_context->FindVariable(name);v.value()) 
	{
		var = v.value();
	}
	else 
	{
		error = ErrorPrefix() +  " undefined variable " + name;
		return {};
	}
	g_context->ir_builder->CreateStore(expr_val, var);
	return { nullptr };
}

optional<llvm::Value*> DeclearExpr::CodeGenerate(string& error) 
{
	if (!mut) {
		Value* assign_value;
		if (!assign) {
			error = ErrorPrefix() + " expect a value for constant variable";
			return {};
		}
		else if (auto v = assign->CodeGenerate(error);!v.has_value()) {
			return {};
		}
		else {
			assign_value = v.value();
		}

		if (!g_context->InsertSSA(name, assign_value)) {
			error = ErrorPrefix() + error;
			return {};
		}
		return { nullptr };
	}

	if (!g_context->InsertVariable(name, type, error)) {
		error = ErrorPrefix() + error;
		return {};
	}

	if (assign != nullptr) 
	{
		auto _var = g_context->FindVariable(name);
		assert(_var.has_value());
		AllocaInst* var = _var.value();

		if (auto v = assign->CodeGenerate(error);v.has_value()) 
		{
			g_context->ir_builder->CreateStore(v.value(),var);
		}
		else 
		{
			return {};
		}
	}

	return { nullptr };
}

optional<llvm::Value*> IfExpr::CodeGenerate(string& error) 
{
	Function* func = g_context->ir_builder->GetInsertBlock()->getParent();

	if (then_expr == nullptr) 
	{
		error = ErrorPrefix() + " then expression should not be null";
		return {};
	}
	bool has_else = else_expr != nullptr;
	
	vector<BasicBlock*> then_blocks{ BasicBlock::Create(*g_context->llvm_context, "then", func) };
	vector<BasicBlock*> then_end_blocks;
	vector<ptr<Expr>> then_block_cond{ cond };
	vector<ptr<BodyExpr>> then_block_body{then_expr};
	for (u32 i = 0; i < elif_expr.size();i++) {
		then_end_blocks.push_back(BasicBlock::Create(*g_context->llvm_context, "then_end", func));
		then_blocks.push_back(BasicBlock::Create(*g_context->llvm_context, "then", func));
		then_block_cond.push_back(elif_cond_expr[i]);
		then_block_body.push_back(elif_expr[i]);
	}
	BasicBlock* else_block;
	if (has_else)
		else_block = BasicBlock::Create(*g_context->llvm_context, "else", func);
	BasicBlock* endif_block = BasicBlock::Create(*g_context->llvm_context, "endif", func);
	if (has_else) {
		then_end_blocks.push_back(else_block);
	}
	else {
		then_end_blocks.push_back(endif_block);
	}

	for (u32 i = 0; i < then_blocks.size();i++) {
		Value* vcond;
		if (auto v = then_block_cond[i]->CodeGenerate(error); v.has_value()) {
			vcond = v.value();
		}
		else {
			return {};
		}
		g_context->ir_builder->CreateCondBr(vcond, then_blocks[i], then_end_blocks[i]);
		g_context->ir_builder->SetInsertPoint(then_blocks[i]);
		if (!then_block_body[i]->GenerateBodyCode(error,false)) {
			return {};
		}
		g_context->ir_builder->CreateBr(endif_block);
		g_context->ir_builder->SetInsertPoint(then_end_blocks[i]);
	}
	if (has_else) {
		if (!else_expr->GenerateBodyCode(error,false)) {
			return {};
		}
		g_context->ir_builder->CreateBr(endif_block);
	}
	g_context->ir_builder->SetInsertPoint(endif_block);
	return { nullptr };
}