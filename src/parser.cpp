#include "parser.hpp"

Parser::Parser(Tokenizer &tokenizer) : tokenizer(tokenizer) {}

void Parser::next() {
	current = tokenizer.next();
}

Token Parser::match(TokenType type) {
	if (current.type != type) {
		error("Expect " + to_string(type) + ", got " + to_string(current.type));
	}
	Token matched = std::move(current);
	next();
	return matched;
}

void Parser::error(const std::string &msg) {
	throw CompileException(current.position, msg);
}

std::unique_ptr<ProgramNode> Parser::parse() {
	next();
	auto ast = parseProgram();
	match(TokenType::END_OF_FILE);
	return ast;
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
	auto ast = std::make_unique<ProgramNode>();
	ast->variables = parseVarDeclares();
	match(TokenType::SEMICOLON);
	ast->statements = parseStatements();
	return ast;
}

std::unique_ptr<VariableDeclarationNode> Parser::parseVarDeclares() {
	auto ast = std::make_unique<VariableDeclarationNode>();
	parseVarType(*ast);
	parseIdentifierList(*ast);
	return ast;
}

void Parser::parseVarType(VariableDeclarationNode &parent) {
	auto type = match(TokenType::KEYWORD_STRING);
	parent.type = std::move(type.str);
}

void Parser::parseIdentifierList(VariableDeclarationNode &parent) {
	auto identifier = match(TokenType::IDENTIFIER);
	parent.identifiers.push_back(std::move(identifier.str));
	parseIdentifierListMore(parent);
}

void Parser::parseIdentifierListMore(VariableDeclarationNode &parent) {
	switch (current.type) {

	case TokenType::COMMA: {
		match(TokenType::COMMA);
		auto identifier = match(TokenType::IDENTIFIER);
		parent.identifiers.push_back(std::move(identifier.str));
		parseIdentifierListMore(parent);
		return;
	}

	case TokenType::SEMICOLON:
		return;

	default:
		error("Expect COMMA or SEMICOLON, got " + to_string(current.type));
	}
}

std::unique_ptr<StatementsNode> Parser::parseStatements() {
	auto ast = std::make_unique<StatementsNode>();
	ast->statements.push_back(parseStatement());
	match(TokenType::SEMICOLON);
	parseStatementsMore(*ast);
	return ast;
}

void Parser::parseStatementsMore(StatementsNode &parent) {
	switch (current.type) {

	case TokenType::IDENTIFIER:
	case TokenType::KEYWORD_IF:
	case TokenType::KEYWORD_DO: {
		parent.statements.push_back(parseStatement());
		match(TokenType::SEMICOLON);
		parseStatementsMore(parent);
		return;
	}

	case TokenType::END_OF_FILE:
	case TokenType::KEYWORD_END:
		return;

	default:
		error("Expect IDENTIFIER, KEYWORD_IF, KEYWORD_DO, END_OF_FILE or "
		      "KEYWORD_END, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
	switch (current.type) {

	case TokenType::IDENTIFIER: {
		return parseAssignStatement();
	}

	case TokenType::KEYWORD_IF: {
		return parseIfStatement();
	}

	case TokenType::KEYWORD_DO: {
		return parseDoWhileStatement();
	}

	default:
		error("Expect IDENTIFIER, KEYWORD_IF or KEYWORD_DO, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<AssignStatementNode> Parser::parseAssignStatement() {
	auto ast = std::make_unique<AssignStatementNode>();
	auto identifier = match(TokenType::IDENTIFIER);
	ast->variable = std::move(identifier.str);
	match(TokenType::OP_ASSIGNMENT);
	ast->expression = parseExpression();
	return ast;
}

std::unique_ptr<IfStatementNode> Parser::parseIfStatement() {
	auto ast = std::make_unique<IfStatementNode>();
	match(TokenType::KEYWORD_IF);
	match(TokenType::LEFT_BRACKET);
	ast->condition = parseCondition();
	match(TokenType::RIGHT_BRACKET);
	ast->true_action = parseNestedStatement();
	match(TokenType::KEYWORD_ELSE);
	ast->false_action = parseNestedStatement();
	return ast;
}

std::unique_ptr<DoWhileStatementNode> Parser::parseDoWhileStatement() {
	auto ast = std::make_unique<DoWhileStatementNode>();
	match(TokenType::KEYWORD_DO);
	ast->loop_action = parseNestedStatement();
	match(TokenType::KEYWORD_WHILE);
	match(TokenType::LEFT_BRACKET);
	ast->condition = parseCondition();
	match(TokenType::RIGHT_BRACKET);
	return ast;
}

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
	auto ast = std::make_unique<ExpressionNode>();
	ast->items.push_back(parseItem());
	parseExpressionMore(*ast);
	return ast;
}

void Parser::parseExpressionMore(ExpressionNode &parent) {
	switch (current.type) {

	case TokenType::OP_CONCAT: {
		match(TokenType::OP_CONCAT);
		parent.items.push_back(parseItem());
		parseExpressionMore(parent);
		return;
	}

	case TokenType::SEMICOLON:
	case TokenType::KEYWORD_ELSE:
	case TokenType::KEYWORD_WHILE:
	case TokenType::RIGHT_BRACKET:
	case TokenType::OP_LESS:
	case TokenType::OP_GREATER:
	case TokenType::OP_NOT_EQUAL:
	case TokenType::OP_GREATER_EQUAL:
	case TokenType::OP_LESS_EQUAL:
	case TokenType::OP_EQUAL:
		return;

	default:
		error("Expect OP_CONCAT, SEMICOLON, KEYWORD_ELSE, KEYWORD_WHILE, "
		      "RIGHT_BRACKET, OP_LESS, OP_GREATER, OP_NOT_EQUAL, "
		      "OP_GREATER_EQUAL, OP_LESS_EQUAL or OP_EQUAL, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<ItemNode> Parser::parseItem() {
	auto ast = std::make_unique<ItemNode>();
	ast->factor = parseFactor();
	parseItemMore(*ast);
	return ast;
}

void Parser::parseItemMore(ItemNode &parent) {
	switch (current.type) {

	case TokenType::OP_REPEAT: {
		match(TokenType::OP_REPEAT);
		auto repeat_time = match(TokenType::NUMBER);
		parent.repeat_times.push_back(std::stoi(repeat_time.str));
		parseItemMore(parent);
		return;
	}

	case TokenType::OP_CONCAT:
	case TokenType::SEMICOLON:
	case TokenType::KEYWORD_ELSE:
	case TokenType::KEYWORD_WHILE:
	case TokenType::RIGHT_BRACKET:
	case TokenType::OP_LESS:
	case TokenType::OP_GREATER:
	case TokenType::OP_NOT_EQUAL:
	case TokenType::OP_GREATER_EQUAL:
	case TokenType::OP_LESS_EQUAL:
	case TokenType::OP_EQUAL:
		return;

	default:
		error("Expect OP_REPEAT, OP_CONCAT, SEMICOLON, KEYWORD_ELSE, "
		      "KEYWORD_WHILE, RIGHT_BRACKET, OP_LESS, OP_GREATER, "
		      "OP_NOT_EQUAL, OP_GREATER_EQUAL, OP_LESS_EQUAL or OP_EQUAL, got" +
		      to_string(current.type));
	}
}

std::unique_ptr<FactorNode> Parser::parseFactor() {
	switch (current.type) {

	case TokenType::IDENTIFIER: {
		auto ast = std::make_unique<VariableFactorNode>();
		auto identifier = match(TokenType::IDENTIFIER);
		ast->identifier = std::move(identifier.str);
		return ast;
	}

	case TokenType::STRING: {
		auto ast = std::make_unique<StringFactorNode>();
		auto string = match(TokenType::STRING);
		auto &raw = string.str;
		ast->str = raw.substr(1, raw.size() - 2); // cut ""
		return ast;
	}

	case TokenType::LEFT_BRACKET: {
		auto ast = std::make_unique<ExpressionFactorNode>();
		match(TokenType::LEFT_BRACKET);
		ast->expression = parseExpression();
		match(TokenType::RIGHT_BRACKET);
		return ast;
	}

	default:
		error("Expect IDENTIFIER, STRING or LEFT_BRACKET, got " +
		      to_string(current.type));
	}
}

RelationOp Parser::parseRelationOp() {
	switch (current.type) {

	case TokenType::OP_LESS: {
		match(TokenType::OP_LESS);
		return RelationOp::LESS;
	}

	case TokenType::OP_GREATER: {
		match(TokenType::OP_GREATER);
		return RelationOp::GREATER;
	}

	case TokenType::OP_NOT_EQUAL: {
		match(TokenType::OP_NOT_EQUAL);
		return RelationOp::NOT_EQUAL;
	}

	case TokenType::OP_GREATER_EQUAL: {
		match(TokenType::OP_GREATER_EQUAL);
		return RelationOp::GREATER_EQUAL;
	}

	case TokenType::OP_LESS_EQUAL: {
		match(TokenType::OP_LESS_EQUAL);
		return RelationOp::LESS_EQUAL;
	}

	case TokenType::OP_EQUAL: {
		match(TokenType::OP_EQUAL);
		return RelationOp::EQUAL;
	}

	default:
		error("Expect OP_LESS, OP_GREATER, OP_NOT_EQUAL, OP_GREATER_EQUAL, "
		      "OP_LESS_EQUAL or OP_EQUAL, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<ConditionNode> Parser::parseCondition() {
	auto ast = std::make_unique<ConditionNode>();
	ast->lhs = parseExpression();
	ast->op = parseRelationOp();
	ast->rhs = parseExpression();
	return ast;
}

std::unique_ptr<StatementsNode> Parser::parseCompoundStatement() {
	match(TokenType::KEYWORD_START);
	auto ast = parseStatements();
	match(TokenType::KEYWORD_END);
	return ast;
}

std::unique_ptr<StatementsNode> Parser::parseNestedStatement() {
	switch (current.type) {

	case TokenType::IDENTIFIER:
	case TokenType::KEYWORD_IF:
	case TokenType::KEYWORD_DO: {
		auto ast = std::make_unique<StatementsNode>();
		ast->statements.push_back(parseStatement());
		return ast;
	}

	case TokenType::KEYWORD_START: {
		return parseCompoundStatement();
	}

	default:
		error(
		    "Expect IDENTIFIER, KEYWORD_IF, KEYWORD_DO or KEYWORD_START, got " +
		    to_string(current.type));
	}
}
