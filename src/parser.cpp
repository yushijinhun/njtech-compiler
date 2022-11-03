#include "parser.hpp"
#include "error.hpp"

namespace compiler {

Parser::Parser(Tokenizer &tokenizer)
    : Parser([&tokenizer] { return tokenizer.next(); }) {}

Parser::Parser(std::function<Token()> tokenizer) : tokenizer(tokenizer) {}

void Parser::next() {
	last_token_end = current.position + current.str.size();
	current = tokenizer();
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
	if (current.type != TokenType::END_OF_FILE) {
		error("Expect end of file");
	}
	return ast;
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
	logp("<PROGRAM> ::= <VAR_DECLARES> SEMICOLON <STATEMENTS>");
	auto ast = std::make_unique<ProgramNode>();
	ast->position_begin = current.position;
	ast->variables = parseVarDeclares();
	match(TokenType::SEMICOLON);
	ast->statements = parseStatements();
	ast->position_end = last_token_end;
	return ast;
}

std::unique_ptr<VariableDeclarationNode> Parser::parseVarDeclares() {
	logp("<VAR_DECLARES> ::= <VAR_TYPE> <IDENTIFIER_LIST>");
	auto ast = std::make_unique<VariableDeclarationNode>();
	ast->position_begin = current.position;
	parseVarType(*ast);
	parseIdentifierList(*ast);
	ast->position_end = last_token_end;
	return ast;
}

void Parser::parseVarType(VariableDeclarationNode &parent) {
	logp("<VAR_TYPE> ::= KEYWORD_STRING");
	auto type = match(TokenType::KEYWORD_STRING);
	parent.type = std::move(type.str);
}

void Parser::parseIdentifierList(VariableDeclarationNode &parent) {
	logp("<IDENTIFIER_LIST> ::= IDENTIFIER <IDENTIFIER_LIST_MORE>");
	auto identifier = match(TokenType::IDENTIFIER);
	parent.identifiers.push_back(std::move(identifier.str));
	parseIdentifierListMore(parent);
}

void Parser::parseIdentifierListMore(VariableDeclarationNode &parent) {
	switch (current.type) {

	case TokenType::COMMA: {
		logp("<IDENTIFIER_LIST_MORE> ::= COMMA IDENTIFIER "
		     "<IDENTIFIER_LIST_MORE>");
		match(TokenType::COMMA);
		auto identifier = match(TokenType::IDENTIFIER);
		parent.identifiers.push_back(std::move(identifier.str));
		parseIdentifierListMore(parent);
		return;
	}

	case TokenType::SEMICOLON:
		logp("<IDENTIFIER_LIST_MORE> ::= ε");
		return;

	default:
		error("Expect COMMA or SEMICOLON, got " + to_string(current.type));
	}
}

std::unique_ptr<StatementsNode> Parser::parseStatements() {
	logp("<STATEMENTS> ::= <STATEMENT> SEMICOLON <STATEMENTS_MORE>");
	auto ast = std::make_unique<StatementsNode>();
	ast->position_begin = current.position;
	ast->statements.push_back(parseStatement());
	match(TokenType::SEMICOLON);
	parseStatementsMore(*ast);
	ast->position_end = last_token_end;
	return ast;
}

void Parser::parseStatementsMore(StatementsNode &parent) {
	switch (current.type) {

	case TokenType::IDENTIFIER:
	case TokenType::KEYWORD_IF:
	case TokenType::KEYWORD_DO: {
		logp("<STATEMENTS_MORE> ::= <STATEMENT> SEMICOLON <STATEMENTS_MORE>");
		parent.statements.push_back(parseStatement());
		match(TokenType::SEMICOLON);
		parseStatementsMore(parent);
		return;
	}

	case TokenType::END_OF_FILE:
	case TokenType::KEYWORD_END:
		logp("<STATEMENTS_MORE> ::= ε");
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
		logp("<STATEMENT> ::= <ASSIGN_STATEMENT>");
		return parseAssignStatement();
	}

	case TokenType::KEYWORD_IF: {
		logp("<STATEMENT> ::= <IF_STATEMENT>");
		return parseIfStatement();
	}

	case TokenType::KEYWORD_DO: {
		logp("<STATEMENT> ::= <WHILE_STATEMENT>");
		return parseDoWhileStatement();
	}

	default:
		error("Expect IDENTIFIER, KEYWORD_IF or KEYWORD_DO, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<AssignStatementNode> Parser::parseAssignStatement() {
	logp("<ASSIGN_STATEMENT> ::= IDENTIFIER OP_ASSIGNMENT <EXPRESSION>");
	auto ast = std::make_unique<AssignStatementNode>();
	ast->position_begin = current.position;
	auto identifier = match(TokenType::IDENTIFIER);
	ast->variable = std::move(identifier.str);
	match(TokenType::OP_ASSIGNMENT);
	ast->expression = parseExpression();
	ast->position_end = last_token_end;
	return ast;
}

std::unique_ptr<IfStatementNode> Parser::parseIfStatement() {
	logp("<IF_STATEMENT> ::= KEYWORD_IF LEFT_BRACKET <CONDITION> RIGHT_BRACKET "
	     "<NESTED_STATEMENT> KEYWORD_ELSE <NESTED_STATEMENT>");
	auto ast = std::make_unique<IfStatementNode>();
	ast->position_begin = current.position;
	match(TokenType::KEYWORD_IF);
	match(TokenType::LEFT_BRACKET);
	ast->condition = parseCondition();
	match(TokenType::RIGHT_BRACKET);
	ast->true_action = parseNestedStatement();
	match(TokenType::KEYWORD_ELSE);
	ast->false_action = parseNestedStatement();
	ast->position_end = last_token_end;
	return ast;
}

std::unique_ptr<DoWhileStatementNode> Parser::parseDoWhileStatement() {
	logp("<WHILE_STATEMENT> ::= KEYWORD_DO <NESTED_STATEMENT> KEYWORD_WHILE "
	     "LEFT_BRACKET <CONDITION> RIGHT_BRACKET");
	auto ast = std::make_unique<DoWhileStatementNode>();
	ast->position_begin = current.position;
	match(TokenType::KEYWORD_DO);
	ast->loop_action = parseNestedStatement();
	match(TokenType::KEYWORD_WHILE);
	match(TokenType::LEFT_BRACKET);
	ast->condition = parseCondition();
	match(TokenType::RIGHT_BRACKET);
	ast->position_end = last_token_end;
	return ast;
}

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
	logp("<EXPRESSION> ::= <ITEM> <EXPRESSION_MORE>");
	auto ast = std::make_unique<ExpressionNode>();
	ast->position_begin = current.position;
	ast->items.push_back(parseItem());
	parseExpressionMore(*ast);
	ast->position_end = last_token_end;
	return ast;
}

void Parser::parseExpressionMore(ExpressionNode &parent) {
	switch (current.type) {

	case TokenType::OP_CONCAT: {
		logp("<EXPRESSION_MORE> ::= OP_CONCAT <ITEM> <EXPRESSION_MORE>");
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
		logp("<EXPRESSION_MORE> ::= ε");
		return;

	default:
		error("Expect OP_CONCAT, SEMICOLON, KEYWORD_ELSE, KEYWORD_WHILE, "
		      "RIGHT_BRACKET, OP_LESS, OP_GREATER, OP_NOT_EQUAL, "
		      "OP_GREATER_EQUAL, OP_LESS_EQUAL or OP_EQUAL, got " +
		      to_string(current.type));
	}
}

std::unique_ptr<ItemNode> Parser::parseItem() {
	logp("<ITEM> ::= <FACTOR> <ITEM_MORE>");
	auto ast = std::make_unique<ItemNode>();
	ast->position_begin = current.position;
	ast->factor = parseFactor();
	parseItemMore(*ast);
	ast->position_end = last_token_end;
	return ast;
}

void Parser::parseItemMore(ItemNode &parent) {
	switch (current.type) {

	case TokenType::OP_REPEAT: {
		logp("<ITEM_MORE> ::= OP_REPEAT NUMBER <ITEM_MORE>");
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
		logp("<ITEM_MORE> ::= ε");
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
		logp("<FACTOR> ::= IDENTIFIER");
		auto ast = std::make_unique<VariableFactorNode>();
		ast->position_begin = current.position;
		auto identifier = match(TokenType::IDENTIFIER);
		ast->identifier = std::move(identifier.str);
		ast->position_end = last_token_end;
		return ast;
	}

	case TokenType::STRING: {
		logp("<FACTOR> ::= STRING");
		auto ast = std::make_unique<StringFactorNode>();
		ast->position_begin = current.position;
		auto string = match(TokenType::STRING);
		auto &raw = string.str;
		ast->str = raw.substr(1, raw.size() - 2); // cut ""
		ast->position_end = last_token_end;
		return ast;
	}

	case TokenType::LEFT_BRACKET: {
		logp("<FACTOR> ::= LEFT_BRACKET <EXPRESSION> RIGHT_BRACKET");
		auto ast = std::make_unique<ExpressionFactorNode>();
		ast->position_begin = current.position;
		match(TokenType::LEFT_BRACKET);
		ast->expression = parseExpression();
		match(TokenType::RIGHT_BRACKET);
		ast->position_end = last_token_end;
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
		logp("<RELATION_OP> ::= OP_LESS");
		match(TokenType::OP_LESS);
		return RelationOp::LESS;
	}

	case TokenType::OP_GREATER: {
		logp("<RELATION_OP> ::= OP_GREATER");
		match(TokenType::OP_GREATER);
		return RelationOp::GREATER;
	}

	case TokenType::OP_NOT_EQUAL: {
		logp("<RELATION_OP> ::= OP_NOT_EQUAL");
		match(TokenType::OP_NOT_EQUAL);
		return RelationOp::NOT_EQUAL;
	}

	case TokenType::OP_GREATER_EQUAL: {
		logp("<RELATION_OP> ::= OP_GREATER_EQUAL");
		match(TokenType::OP_GREATER_EQUAL);
		return RelationOp::GREATER_EQUAL;
	}

	case TokenType::OP_LESS_EQUAL: {
		logp("<RELATION_OP> ::= OP_LESS_EQUAL");
		match(TokenType::OP_LESS_EQUAL);
		return RelationOp::LESS_EQUAL;
	}

	case TokenType::OP_EQUAL: {
		logp("<RELATION_OP> ::= OP_EQUAL");
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
	logp("<CONDITION> ::= <EXPRESSION> <RELATION_OP> <EXPRESSION>");
	auto ast = std::make_unique<ConditionNode>();
	ast->position_begin = current.position;
	ast->lhs = parseExpression();
	ast->op = parseRelationOp();
	ast->rhs = parseExpression();
	ast->position_end = last_token_end;
	return ast;
}

std::unique_ptr<StatementsNode> Parser::parseCompoundStatement() {
	logp("<COMPOUND_STATEMENT> ::= KEYWORD_START <STATEMENTS> KEYWORD_END");
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
		logp("<NESTED_STATEMENT> ::= <STATEMENT>");
		auto ast = std::make_unique<StatementsNode>();
		ast->position_begin = current.position;
		ast->statements.push_back(parseStatement());
		ast->position_end = last_token_end;
		return ast;
	}

	case TokenType::KEYWORD_START: {
		logp("<NESTED_STATEMENT> ::= <COMPOUND_STATEMENT>");
		return parseCompoundStatement();
	}

	default:
		error(
		    "Expect IDENTIFIER, KEYWORD_IF, KEYWORD_DO or KEYWORD_START, got " +
		    to_string(current.type));
	}
}

void Parser::logp(const std::string &p) {
	if (production_cb != nullptr) {
		production_cb(p);
	}
}

void Parser::set_production_callback(
    std::function<void(const std::string &)> cb) {
	this->production_cb = cb;
}

void Parser::set_print_production_to(std::ostream &out) {
	this->production_cb = [&out](const std::string &production) {
		out << production << "\n";
	};
}

} // namespace compiler
