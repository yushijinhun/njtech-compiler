#include "tokenizer.hpp"
#include "error.hpp"

namespace compiler {

std::string to_string(TokenType x) {
	switch (x) {
	case TokenType::LEFT_BRACKET:
		return "LEFT_BRACKET";
	case TokenType::RIGHT_BRACKET:
		return "RIGHT_BRACKET";
	case TokenType::SEMICOLON:
		return "SEMICOLON";
	case TokenType::COMMA:
		return "COMMA";
	case TokenType::OP_CONCAT:
		return "OP_CONCAT";
	case TokenType::OP_REPEAT:
		return "OP_REPEAT";
	case TokenType::OP_LESS:
		return "OP_LESS";
	case TokenType::OP_NOT_EQUAL:
		return "OP_NOT_EQUAL";
	case TokenType::OP_LESS_EQUAL:
		return "OP_LESS_EQUAL";
	case TokenType::OP_GREATER:
		return "OP_GREATER";
	case TokenType::OP_GREATER_EQUAL:
		return "OP_GREATER_EQUAL";
	case TokenType::OP_ASSIGNMENT:
		return "OP_ASSIGNMENT";
	case TokenType::OP_EQUAL:
		return "OP_EQUAL";
	case TokenType::KEYWORD_STRING:
		return "KEYWORD_STRING";
	case TokenType::KEYWORD_START:
		return "KEYWORD_START";
	case TokenType::KEYWORD_ELSE:
		return "KEYWORD_ELSE";
	case TokenType::KEYWORD_END:
		return "KEYWORD_END";
	case TokenType::KEYWORD_WHILE:
		return "KEYWORD_WHILE";
	case TokenType::KEYWORD_IF:
		return "KEYWORD_IF";
	case TokenType::KEYWORD_DO:
		return "KEYWORD_DO";
	case TokenType::IDENTIFIER:
		return "IDENTIFIER";
	case TokenType::NUMBER:
		return "NUMBER";
	case TokenType::STRING:
		return "STRING";
	case TokenType::END_OF_FILE:
		return "END_OF_FILE";
	default:
		throw std::runtime_error("Unexpected token type");
	}
}

enum class Tokenizer::State {
	N_BEGIN,
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
	N_S,
	N_ST,
	N_STR,
	N_STRI,
	N_STRIN,
	KEYWORD_STRING,
	N_STA,
	N_STAR,
	KEYWORD_START,
	N_E,
	N_EL,
	N_ELS,
	KEYWORD_ELSE,
	N_EN,
	KEYWORD_END,
	N_W,
	N_WH,
	N_WHI,
	N_WHIL,
	KEYWORD_WHILE,
	N_I,
	KEYWORD_IF,
	N_D,
	KEYWORD_DO,
	IDENTIFIER,
	NUMBER,
	N_STRING_INCOMPLETE,
	STRING,
};

Tokenizer::Tokenizer(std::function<char()> source)
    : source(source), current_ch('\0'), position(-1), state(State::N_BEGIN) {}

Tokenizer::Tokenizer(std::istream &in)
    : Tokenizer([&in] {
	      char ch;
	      if (in.get(ch)) {
		      return ch;
	      } else {
		      return '\0';
	      }
      }) {}

char Tokenizer::read() {
	if (back_ch.has_value()) {
		current_ch = *back_ch;
		back_ch = std::nullopt;
	} else {
		current_ch = source();
	}
	position++;
	buf.push_back(current_ch);
	return current_ch;
}

void Tokenizer::back() {
	if (back_ch.has_value()) {
		throw std::runtime_error(
		    "Going back more than 1 character is unsupported!");
	} else if (position < 0) {
		throw std::runtime_error("No character has been read yet!");
	} else if (buf.empty()) {
		throw std::runtime_error("Token buffer is empty, can't go back!");
	}
	position--;
	back_ch = current_ch;
	buf.pop_back();
}

void Tokenizer::error(const std::string &error) {
	throw CompileException(position, error);
}

Token Tokenizer::emit(TokenType type) {
	Token token = {.type = type,
	               .str = std::string(buf.begin(), buf.end()),
	               .position = position - static_cast<int>(buf.size()) + 1};
	state = State::N_BEGIN;
	buf.clear();
	return token;
}

static constexpr bool is_digit(char ch) {
	return ch >= '0' && ch <= '9';
}

static constexpr bool is_letter(char ch) {
	return ch >= 'a' && ch <= 'z';
}

Token Tokenizer::next() {
	for (;;) {
		char ch = read();
		switch (state) {

		case State::N_BEGIN:
			if (ch == '\0') {
				back();
				return emit(TokenType::END_OF_FILE);
			} else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
				buf.pop_back(); // skip whitespaces
				state = State::N_BEGIN;
			} else if (ch == '(') {
				state = State::LEFT_BRACKET;
			} else if (ch == ')') {
				state = State::RIGHT_BRACKET;
			} else if (ch == ';') {
				state = State::SEMICOLON;
			} else if (ch == ',') {
				state = State::COMMA;
			} else if (ch == '+') {
				state = State::OP_CONCAT;
			} else if (ch == '*') {
				state = State::OP_REPEAT;
			} else if (ch == '<') {
				state = State::OP_LESS;
			} else if (ch == '>') {
				state = State::OP_GREATER;
			} else if (ch == '=') {
				state = State::OP_ASSIGNMENT;
			} else if (ch == 's') {
				state = State::N_S;
			} else if (ch == 'e') {
				state = State::N_E;
			} else if (ch == 'w') {
				state = State::N_W;
			} else if (ch == 'i') {
				state = State::N_I;
			} else if (ch == 'd') {
				state = State::N_D;
			} else if (is_digit(ch)) {
				state = State::NUMBER;
			} else if (ch == '"') {
				state = State::N_STRING_INCOMPLETE;
			} else if (is_letter(ch)) {
				state = State::IDENTIFIER;
			} else {
				error("Unrecognized character");
			}
			break;

		case State::LEFT_BRACKET:
			back();
			return emit(TokenType::LEFT_BRACKET);

		case State::RIGHT_BRACKET:
			back();
			return emit(TokenType::RIGHT_BRACKET);

		case State::SEMICOLON:
			back();
			return emit(TokenType::SEMICOLON);

		case State::COMMA:
			back();
			return emit(TokenType::COMMA);

		case State::OP_CONCAT:
			back();
			return emit(TokenType::OP_CONCAT);

		case State::OP_REPEAT:
			back();
			return emit(TokenType::OP_REPEAT);

		case State::OP_LESS:
			if (ch == '>') {
				state = State::OP_NOT_EQUAL;
			} else if (ch == '=') {
				state = State::OP_LESS_EQUAL;
			} else {
				back();
				return emit(TokenType::OP_LESS);
			}
			break;

		case State::OP_NOT_EQUAL:
			back();
			return emit(TokenType::OP_NOT_EQUAL);

		case State::OP_LESS_EQUAL:
			back();
			return emit(TokenType::OP_LESS_EQUAL);

		case State::OP_GREATER:
			if (ch == '=') {
				state = State::OP_GREATER_EQUAL;
			} else {
				back();
				return emit(TokenType::OP_GREATER);
			}
			break;

		case State::OP_GREATER_EQUAL:
			back();
			return emit(TokenType::OP_GREATER_EQUAL);

		case State::OP_ASSIGNMENT:
			if (ch == '=') {
				state = State::OP_EQUAL;
			} else {
				back();
				return emit(TokenType::OP_ASSIGNMENT);
			}
			break;

		case State::OP_EQUAL:
			back();
			return emit(TokenType::OP_EQUAL);

		case State::N_S:
			if (ch == 't') {
				state = State::N_ST;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_ST:
			if (ch == 'r') {
				state = State::N_STR;
			} else if (ch == 'a') {
				state = State::N_STA;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_STR:
			if (ch == 'i') {
				state = State::N_STRI;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_STRI:
			if (ch == 'n') {
				state = State::N_STRIN;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_STRIN:
			if (ch == 'g') {
				state = State::KEYWORD_STRING;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_STRING:
			back();
			return emit(TokenType::KEYWORD_STRING);

		case State::N_STA:
			if (ch == 'r') {
				state = State::N_STAR;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_STAR:
			if (ch == 't') {
				state = State::KEYWORD_START;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_START:
			back();
			return emit(TokenType::KEYWORD_START);

		case State::N_E:
			if (ch == 'l') {
				state = State::N_EL;
			} else if (ch == 'n') {
				state = State::N_EN;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_EL:
			if (ch == 's') {
				state = State::N_ELS;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_ELS:
			if (ch == 'e') {
				state = State::KEYWORD_ELSE;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_ELSE:
			back();
			return emit(TokenType::KEYWORD_ELSE);

		case State::N_EN:
			if (ch == 'd') {
				state = State::KEYWORD_END;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_END:
			back();
			return emit(TokenType::KEYWORD_END);

		case State::N_W:
			if (ch == 'h') {
				state = State::N_WH;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_WH:
			if (ch == 'i') {
				state = State::N_WHI;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_WHI:
			if (ch == 'l') {
				state = State::N_WHIL;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::N_WHIL:
			if (ch == 'e') {
				state = State::KEYWORD_WHILE;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_WHILE:
			back();
			return emit(TokenType::KEYWORD_WHILE);

		case State::N_I:
			if (ch == 'f') {
				state = State::KEYWORD_IF;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_IF:
			back();
			return emit(TokenType::KEYWORD_IF);

		case State::N_D:
			if (ch == 'o') {
				state = State::KEYWORD_DO;
			} else if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::KEYWORD_DO:
			back();
			return emit(TokenType::KEYWORD_DO);

		case State::IDENTIFIER:
			if (is_letter(ch) || is_digit(ch)) {
				state = State::IDENTIFIER;
			} else {
				back();
				return emit(TokenType::IDENTIFIER);
			}
			break;

		case State::NUMBER:
			back();
			return emit(TokenType::NUMBER);

		case State::N_STRING_INCOMPLETE:
			if (is_letter(ch)) {
				state = State::N_STRING_INCOMPLETE;
			} else if (ch == '"') {
				state = State::STRING;
			} else {
				error("Unexpected character in string");
			}
			break;

		case State::STRING:
			back();
			return emit(TokenType::STRING);

		default:
			throw std::runtime_error("Tokenizer is in unexpected state!");
		}
	}
}

} // namespace compiler
