#include "tokenizer.hpp"
#include <iomanip>
#include <iostream>

int main() {
	Tokenizer tokenizer([] {
		char ch;
		if (std::cin.get(ch)) {
			return ch;
		} else {
			return '\0';
		}
	});
	try {
		for (;;) {
			Token token;
			token = tokenizer.next();
			std::cout << std::left << std::setw(20) << to_string(token.type)
			          << token.str << "\n";
			if (token.type == TokenType::END_OF_FILE)
				break;
		}
	} catch (LexicalException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
