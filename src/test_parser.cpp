#include "parser.hpp"
#include <iomanip>
#include <iostream>

int main() {
	try {
		Tokenizer tokenizer(std::cin);
		Parser parser(tokenizer);
		parser.parse();
		std::cout << "Accept\n";
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
