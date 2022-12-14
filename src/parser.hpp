#pragma once

#include "ast.hpp"
#include "tokenizer.hpp"
#include <memory>

namespace compiler {

class Parser {
  public:
	explicit Parser(Tokenizer &tokenizer);
	explicit Parser(std::function<Token()> tokenizer);
	Parser(const Parser &) = delete;

	void set_production_callback(std::function<void(const std::string &)> cb);
	void set_print_production_to(std::ostream &out);

	std::unique_ptr<ProgramNode> parse();

  private:
	std::function<Token()> tokenizer;
	int last_token_end;
	Token current;
	std::function<void(const std::string &)> production_cb;

	void next();
	Token match(TokenType type);
	[[noreturn]] void error(const std::string &msg);
	void logp(const std::string &msg);

	std::unique_ptr<ProgramNode> parseProgram();
	std::unique_ptr<VariableDeclarationNode> parseVarDeclares();
	void parseVarType(VariableDeclarationNode &parent);
	void parseIdentifierList(VariableDeclarationNode &parent);
	void parseIdentifierListMore(VariableDeclarationNode &parent);
	std::unique_ptr<StatementsNode> parseStatements();
	void parseStatementsMore(StatementsNode &parent);
	std::unique_ptr<StatementNode> parseStatement();
	std::unique_ptr<AssignStatementNode> parseAssignStatement();
	std::unique_ptr<IfStatementNode> parseIfStatement();
	std::unique_ptr<DoWhileStatementNode> parseDoWhileStatement();
	std::unique_ptr<ExpressionNode> parseExpression();
	void parseExpressionMore(ExpressionNode &parent);
	std::unique_ptr<ItemNode> parseItem();
	void parseItemMore(ItemNode &parent);
	std::unique_ptr<FactorNode> parseFactor();
	RelationOp parseRelationOp();
	std::unique_ptr<ConditionNode> parseCondition();
	std::unique_ptr<StatementsNode> parseCompoundStatement();
	std::unique_ptr<StatementsNode> parseNestedStatement();
};

} // namespace compiler
