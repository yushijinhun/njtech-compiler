#pragma once

#include "ast.hpp"
#include <map>
#include <optional>
#include <variant>

class TAC {
  public:
	struct Variable {
		std::string name;
		std::string type;
		bool temporary;
	};

	struct Literal {
		std::string value;
		std::string type;
	};

	struct Label {
		int num;
	};

	using Value = std::variant<Variable, Literal>;

	struct Instruction {
		using Arg = std::optional<Value>;
		using Result = std::variant<Variable, Label>;

		std::string op;
		Arg arg1;
		Arg arg2;
		Result result;
	};

	std::vector<Instruction> instructions;
	std::map<std::string, Variable> variable_table;
	int tempVariableCount = 0;
	int nextQ = 0;

	explicit TAC(const ProgramNode &ast);

	friend std::ostream &operator<<(std::ostream &out, const TAC &tac);

  private:
	Variable tempVar(const std::string &type);
	Variable lookupVar(const std::string &name, int pos);
	void generate(const std::string &op, Instruction::Arg arg1,
	              Instruction::Arg arg2, Instruction::Result result);

	void translateVariableDeclaration(const VariableDeclarationNode &node);
	void translateStatements(const StatementsNode &node);
	void translateStatement(const StatementNode &node);
	void translateAssignStatement(const AssignStatementNode &node);
	void translateIfStatement(const IfStatementNode &node);
	void translateDoWhileStatement(const DoWhileStatementNode &node);
	Value translateExpression(const ExpressionNode &node);
	Value translateCondition(const ConditionNode &node);
	Value translateItem(const ItemNode &node);
	Value translateFactor(const FactorNode &node);
	Value translateStringFactor(const StringFactorNode &node);
	Value translateVariableFactor(const VariableFactorNode &node);
	Value translateExpressionFactor(const ExpressionFactorNode &node);
};
