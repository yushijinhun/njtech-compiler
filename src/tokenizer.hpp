#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

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

class LexicalException : public std::exception {
  public:
	const int position;
	const std::string error;

	explicit LexicalException(int position, const std::string &error);

	virtual const char *what() const noexcept;

  private:
	const std::string what_msg;
};

struct Token {
	TokenType type;
	std::string str;
};

class Tokenizer {
  public:
	explicit Tokenizer(std::function<char()> source);

	Token next();

  private:
	enum class State;

	std::function<char()> source;
	std::optional<char> back_ch;
	char current_ch;
	int position;
	std::vector<char> buf;
	State state;

	char read();
	void back();
	void error(const std::string &error);
	Token emit(TokenType type);
};
