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

void Parser::parse() {
	next();
	parseProgram();
	match(TokenType::END_OF_FILE);
}

void Parser::parseProgram() {
	parseVarDeclares();
	match(TokenType::SEMICOLON);
	parseStatements();
}

void Parser::parseVarDeclares() {
	parseVarType();
	parseIdentifierList();
}

void Parser::parseVarType() {
	match(TokenType::KEYWORD_STRING);
}

void Parser::parseIdentifierList() {
	match(TokenType::IDENTIFIER);
	parseIdentifierListMore();
}

void Parser::parseIdentifierListMore() {
	switch (current.type) {

	case TokenType::COMMA: {
		match(TokenType::COMMA);
		match(TokenType::IDENTIFIER);
		parseIdentifierListMore();
	} break;

	case TokenType::SEMICOLON: {
	} break;

	default:
		error("Expect COMMA or SEMICOLON, got " + to_string(current.type));
		break;
	}
}

void Parser::parseStatements() {
	parseStatement();
	match(TokenType::SEMICOLON);
	parseStatementsMore();
}

void Parser::parseStatementsMore() {
	switch (current.type) {

	case TokenType::IDENTIFIER:
	case TokenType::KEYWORD_IF:
	case TokenType::KEYWORD_DO: {
		parseStatement();
		match(TokenType::SEMICOLON);
		parseStatementsMore();
	} break;

	case TokenType::END_OF_FILE:
	case TokenType::KEYWORD_END: {
	} break;

	default:
		error("Expect IDENTIFIER, KEYWORD_IF, KEYWORD_DO, END_OF_FILE or "
		      "KEYWORD_END, got " +
		      to_string(current.type));
		break;
	}
}

void Parser::parseStatement() {
	switch (current.type) {

	case TokenType::IDENTIFIER: {
		parseAssignStatement();
	} break;

	case TokenType::KEYWORD_IF: {
		parseIfStatement();
	} break;

	case TokenType::KEYWORD_DO: {
		parseWhileStatement();
	} break;

	default:
		error("Expect IDENTIFIER, KEYWORD_IF or KEYWORD_DO, got " +
		      to_string(current.type));
		break;
	}
}

void Parser::parseAssignStatement() {
	match(TokenType::IDENTIFIER);
	match(TokenType::OP_ASSIGNMENT);
	parseExpression();
}

void Parser::parseIfStatement() {
	match(TokenType::KEYWORD_IF);
	match(TokenType::LEFT_BRACKET);
	parseCondition();
	match(TokenType::RIGHT_BRACKET);
	parseNestedStatement();
	match(TokenType::KEYWORD_ELSE);
	parseNestedStatement();
}

void Parser::parseWhileStatement() {
	match(TokenType::KEYWORD_DO);
	parseNestedStatement();
	match(TokenType::KEYWORD_WHILE);
	match(TokenType::LEFT_BRACKET);
	parseCondition();
	match(TokenType::RIGHT_BRACKET);
}

void Parser::parseExpression() {
	parseItem();
	parseExpressionMore();
}

void Parser::parseExpressionMore() {
	switch (current.type) {

	case TokenType::OP_CONCAT: {
		match(TokenType::OP_CONCAT);
		parseItem();
		parseExpressionMore();
	} break;

	case TokenType::SEMICOLON:
	case TokenType::KEYWORD_ELSE:
	case TokenType::KEYWORD_WHILE:
	case TokenType::RIGHT_BRACKET:
	case TokenType::OP_LESS:
	case TokenType::OP_GREATER:
	case TokenType::OP_NOT_EQUAL:
	case TokenType::OP_GREATER_EQUAL:
	case TokenType::OP_LESS_EQUAL:
	case TokenType::OP_EQUAL: {
	} break;

	default:
		error("Expect OP_CONCAT, SEMICOLON, KEYWORD_ELSE, KEYWORD_WHILE, "
		      "RIGHT_BRACKET, OP_LESS, OP_GREATER, OP_NOT_EQUAL, "
		      "OP_GREATER_EQUAL, OP_LESS_EQUAL or OP_EQUAL, got " +
		      to_string(current.type));
		break;
	}
}

void Parser::parseItem() {
	parseFactor();
	parseItemMore();
}

void Parser::parseItemMore() {
	switch (current.type) {

	case TokenType::OP_REPEAT: {
		match(TokenType::OP_REPEAT);
		match(TokenType::NUMBER);
		parseItemMore();
	} break;

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
	case TokenType::OP_EQUAL: {
	} break;

	default:
		error("Expect OP_REPEAT, OP_CONCAT, SEMICOLON, KEYWORD_ELSE, "
		      "KEYWORD_WHILE, RIGHT_BRACKET, OP_LESS, OP_GREATER, "
		      "OP_NOT_EQUAL, OP_GREATER_EQUAL, OP_LESS_EQUAL or OP_EQUAL, got" +
		      to_string(current.type));
		break;
	}
}

void Parser::parseFactor() {
	switch (current.type) {

	case TokenType::IDENTIFIER: {
		match(TokenType::IDENTIFIER);
	} break;

	case TokenType::STRING: {
		match(TokenType::STRING);
	} break;

	case TokenType::LEFT_BRACKET: {
		match(TokenType::LEFT_BRACKET);
		parseExpression();
		match(TokenType::RIGHT_BRACKET);
	} break;

	default:
		error("Expect IDENTIFIER, STRING or LEFT_BRACKET, got " +
		      to_string(current.type));
		break;
	}
}

void Parser::parseRelationOp() {
	switch (current.type) {

	case TokenType::OP_LESS: {
		match(TokenType::OP_LESS);
	} break;

	case TokenType::OP_GREATER: {
		match(TokenType::OP_GREATER);
	} break;

	case TokenType::OP_NOT_EQUAL: {
		match(TokenType::OP_NOT_EQUAL);
	} break;

	case TokenType::OP_GREATER_EQUAL: {
		match(TokenType::OP_GREATER_EQUAL);
	} break;

	case TokenType::OP_LESS_EQUAL: {
		match(TokenType::OP_LESS_EQUAL);
	} break;

	case TokenType::OP_EQUAL: {
		match(TokenType::OP_EQUAL);
	} break;

	default:
		error("Expect OP_LESS, OP_GREATER, OP_NOT_EQUAL, OP_GREATER_EQUAL, "
		      "OP_LESS_EQUAL or OP_EQUAL, got " +
		      to_string(current.type));
		break;
	}
}

void Parser::parseCondition() {
	parseExpression();
	parseRelationOp();
	parseExpression();
}

void Parser::parseCompoundStatement() {
	match(TokenType::KEYWORD_START);
	parseStatements();
	match(TokenType::KEYWORD_END);
}

void Parser::parseNestedStatement() {
	switch (current.type) {

	case TokenType::IDENTIFIER:
	case TokenType::KEYWORD_IF:
	case TokenType::KEYWORD_DO: {
		parseStatement();
	} break;

	case TokenType::KEYWORD_START: {
		parseCompoundStatement();
	} break;

	default:
		error(
		    "Expect IDENTIFIER, KEYWORD_IF, KEYWORD_DO or KEYWORD_START, got " +
		    to_string(current.type));
		break;
	}
}
