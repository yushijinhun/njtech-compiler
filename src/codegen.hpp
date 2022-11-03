#pragma once

#include "ast.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <map>

namespace compiler {

class LLVMCodeGen {
  public:
	static std::unique_ptr<llvm::Module> fromAST(llvm::LLVMContext &ctx,
	                                             const ProgramNode &node);

  private:
	explicit LLVMCodeGen(llvm::LLVMContext &ctx);

	struct DestructibleValue {
		llvm::Value *val;
		bool transient;
		std::optional<llvm::Value *> strlen;
	};

	llvm::LLVMContext &ctx;
	std::unique_ptr<llvm::Module> module;
	llvm::IRBuilder<> builder;
	std::map<std::string, llvm::AllocaInst *> variables;

	llvm::Value *genStrlen(llvm::Value *str_ptr);
	llvm::Value *genStrAlloc(llvm::Value *len);
	void genStrFree(llvm::Value *ptr);
	void destructTransientValue(DestructibleValue &&val);
	void genPrintVariables();

	void visitVariableDeclaration(const VariableDeclarationNode &node);
	DestructibleValue visitStringFactor(const StringFactorNode &node);
	DestructibleValue visitVariableFactor(const VariableFactorNode &node);
	DestructibleValue visitExpressionFactor(const ExpressionFactorNode &node);
	DestructibleValue visitFactor(const FactorNode &node);
	DestructibleValue visitItem(const ItemNode &node);
	DestructibleValue visitExpression(const ExpressionNode &node);
	llvm::Value *visitCondition(const ConditionNode &node);
	void visitAssignStatement(const AssignStatementNode &node);
	void visitIfStatement(const IfStatementNode &node);
	void visitDoWhileStatement(const DoWhileStatementNode &node);
	void visitStatement(const StatementNode &node);
	void visitStatements(const StatementsNode &node);
	void visitProgram(const ProgramNode &node);
};

} // namespace compiler
