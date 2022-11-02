#include "ast.hpp"
#include "error.hpp"
#include "parser.hpp"
#include "tac.hpp"
#include <iomanip>
#include <iostream>

int main() {
	try {
		Tokenizer tokenizer(std::cin);
		Parser parser([&tokenizer] {
			auto token = tokenizer.next();
			std::cout << std::left << std::setw(5) << token.position
			          << std::left << std::setw(20) << to_string(token.type)
			          << token.str << "\n";
			return token;
		});
		auto ast = parser.parse();
		std::cout << "\n";
		std::cout << "---- AST ----\n" << *ast << "\n\n";
		std::cout << "---- TAC ----\n" << TAC(*ast) << "\n";
	} catch (CompileException &ex) {
		std::cerr << ex.what() << "\n";
	}
}
