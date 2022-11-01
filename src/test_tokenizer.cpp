#include "tokenizer.hpp"
#include <iomanip>
#include <iostream>

int main() {
	try {
		Tokenizer tokenizer(std::cin);
		for (;;) {
			Token token;
			token = tokenizer.next();
			std::cout << std::left << std::setw(5) << token.position
			          << std::left << std::setw(20) << to_string(token.type)
			          << token.str << "\n";
			if (token.type == TokenType::END_OF_FILE)
				break;
		}
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
