#include "ast.hpp"
#include "parser.hpp"
#include "tac.hpp"
#include <iomanip>
#include <iostream>

int main() {
	try {
		Tokenizer tokenizer(std::cin);
		Parser parser(tokenizer);
		auto ast = parser.parse();
		std::cout << "AST:\n" << *ast << "\n\n";
		std::cout << "TAC:\n" << TAC(*ast) << "\n";
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
