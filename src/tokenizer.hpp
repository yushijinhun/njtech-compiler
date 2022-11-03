#pragma once

#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <string>

namespace compiler {

enum class TokenType {
	LEFT_BRACKET,
	RIGHT_BRACKET,
	SEMICOLON,
	COMMA,
	OP_CONCAT,
	OP_REPEAT,
	OP_LESS,
	OP_NOT_EQUAL,
	OP_LESS_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_ASSIGNMENT,
	OP_EQUAL,
	KEYWORD_STRING,
	KEYWORD_START,
	KEYWORD_ELSE,
	KEYWORD_END,
	KEYWORD_WHILE,
	KEYWORD_IF,
	KEYWORD_DO,
	IDENTIFIER,
	NUMBER,
	STRING,
	END_OF_FILE
};

std::string to_string(TokenType x);

struct Token {
	TokenType type;
	std::string str;
	int position;
};

class Tokenizer {
  public:
	explicit Tokenizer(std::function<char()> source);
	explicit Tokenizer(std::istream &source);
	Tokenizer(const Tokenizer &) = delete;

	Token next();

	void set_token_callback(std::function<void(const Token &)> cb);
	void set_print_token_to(std::ostream &out);

  private:
	enum class State;

	std::function<char()> source;
	std::optional<char> back_ch;
	char current_ch;
	int position;
	std::vector<char> buf;
	State state;
	std::function<void(const Token &)> token_cb;

	char read();
	void back();
	[[noreturn]] void error(const std::string &error);
	Token emit(TokenType type);
};

} // namespace compiler
