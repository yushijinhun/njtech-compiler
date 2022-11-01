#include "parser.hpp"
#include <iomanip>
#include <iostream>

int main() {
	try {
		Tokenizer tokenizer(std::cin);
		Parser parser(tokenizer);
		auto ast = parser.parse();
		std::cout << *ast << "\n";
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
