#pragma once

#include "ast.hpp"
#include <map>
#include <optional>
#include <set>
#include <variant>

class TAC {
  public:
	struct Variable {
		std::string name;
		std::string type;
		bool temporary;

		bool operator<(const Variable &b) const {
			if (temporary != b.temporary) {
				return temporary < b.temporary;
			}
			if (type != b.type) {
				return type < b.type;
			}
			if (name != b.name) {
				return name < b.name;
			}
			return false;
		}
	};

	struct Literal {
		std::string value;
		std::string type;
		bool operator<(const Literal &b) const {
			if (type != b.type) {
				return type < b.type;
			}
			if (value != b.value) {
				return value < b.value;
			}
			return false;
		}
	};

	struct Label {
		int num;
		bool operator<(const Label &b) const {
			return num < b.num;
		}
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
	std::set<Literal> literal_table;
	int tempVariableCount = 0;
	int nextQ = 0;

	explicit TAC(const ProgramNode &ast);

	friend std::ostream &operator<<(std::ostream &out, const TAC &tac);

  private:
	Variable tempVar(const std::string &type);
	Variable lookupVar(const std::string &name, int pos);
	void generate(const std::string &op, Instruction::Arg arg1,
	              Instruction::Arg arg2, Instruction::Result result);
	Literal makeLiteral(const std::string &value, const std::string &type);

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
