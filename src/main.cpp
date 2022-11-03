#include "aot.hpp"
#include "ast.hpp"
#include "codegen.hpp"
#include "error.hpp"
#include "jit.hpp"
#include "parser.hpp"
#include "tac.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <llvm/Support/raw_os_ostream.h>

int main() {
	try {
		compiler::Tokenizer tokenizer(std::cin);
		compiler::Parser parser([&tokenizer] {
			auto token = tokenizer.next();
			std::cout << std::left << std::setw(5) << token.position
			          << std::left << std::setw(20) << to_string(token.type)
			          << token.str << "\n";
			return token;
		});
		auto ast = parser.parse();
		std::cout << "\n";
		std::cout << "---- AST ----\n" << *ast << "\n\n";
		std::cout << "---- TAC ----\n" << compiler::TAC(*ast) << "\n";

		auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
		auto module = compiler::LLVMCodeGen::fromAST(*llvm_ctx, *ast);

		std::cout << "---- LLVM IR ----\n";
		llvm::raw_os_ostream file_stream(std::cout);
		module->print(file_stream, nullptr);
		std::cout << "\n";

		{
			// write to program.ll
			std::cout << "--- Writing LLVM IR to program.ll ----\n";
			std::ofstream fout("program.ll");
			llvm::raw_os_ostream file_stream(fout);
			module->print(file_stream, nullptr);
			std::cout << "\n";
		}

		{
			// write to program.o
			std::cout << "--- Writing object code to program.o ----\n";
			compiler::aot::initialize();
			compiler::aot::generate_object(*module, "program.o");
			std::cout << "\n";
		}

		std::cout << "---- JIT Execution ----\n";
		compiler::jit::initialize();
		compiler::jit::invoke_module(std::move(llvm_ctx), std::move(module));
		std::cout << "\n";

	} catch (compiler::CompileException &ex) {
		std::cout << ex.what() << "\n";
	}
}
