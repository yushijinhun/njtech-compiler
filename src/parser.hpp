#pragma once

#include "tokenizer.hpp"
#include "util.hpp"
#include <memory>

class Parser {
  public:
	explicit Parser(Tokenizer &tokenizer);
	Parser(const Parser &) = delete;

	void parse();

  private:
	Token current;
	void next();
	Token match(TokenType type);
	void error(const std::string &msg);

	void parseProgram();
	void parseVarDeclares();
	void parseVarType();
	void parseIdentifierList();
	void parseIdentifierListMore();
	void parseStatements();
	void parseStatementsMore();
	void parseStatement();
	void parseAssignStatement();
	void parseIfStatement();
	void parseWhileStatement();
	void parseExpression();
	void parseExpressionMore();
	void parseItem();
	void parseItemMore();
	void parseFactor();
	void parseRelationOp();
	void parseCondition();
	void parseCompoundStatement();
	void parseNestedStatement();

  private:
	Tokenizer &tokenizer;
};
