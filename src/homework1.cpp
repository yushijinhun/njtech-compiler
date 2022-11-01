#include "tokenizer.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
	std::ifstream in("in.txt");
	if (in.fail()) {
		std::cerr << "Failed to open in.txt\n";
		return 1;
	}

	std::ofstream debug("debug.txt");
	if (debug.fail()) {
		std::cerr << "Failed to open debug.txt\n";
		return 1;
	}

	Tokenizer tokenizer([&] {
		char ch;
		if (in.get(ch)) {
			return ch;
		} else {
			return '\0';
		}
	});

	try {
		for (;;) {
			Token token;
			token = tokenizer.next();
			debug << std::left << std::setw(20) << to_string(token.type)
			      << token.str << "\n";
			if (token.type == TokenType::END_OF_FILE)
				break;
		}
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
		return 1;
	}
}
