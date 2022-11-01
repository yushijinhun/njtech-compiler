#include "tac.hpp"
#include "util.hpp"

static std::ostream &operator<<(std::ostream &out,
                                const TAC::Instruction::Arg &arg) {
	if (!arg.has_value()) {
		out << "null";
	} else if (std::holds_alternative<TAC::Variable>(*arg)) {
		out << std::get<TAC::Variable>(*arg).name;
	} else if (std::holds_alternative<TAC::Literal>(*arg)) {
		out << std::get<TAC::Literal>(*arg).value;
	} else {
		throw std::bad_variant_access();
	}
	return out;
}

static std::ostream &operator<<(std::ostream &out,
                                const TAC::Instruction::Result &arg) {
	if (std::holds_alternative<TAC::Variable>(arg)) {
		out << std::get<TAC::Variable>(arg).name;
	} else if (std::holds_alternative<TAC::Label>(arg)) {
		out << std::get<TAC::Label>(arg).num;
	} else {
		throw std::bad_variant_access();
	}
	return out;
}

static std::ostream &operator<<(std::ostream &out,
                                const TAC::Instruction &instruction) {
	out << "(" << instruction.op << ", " << instruction.arg1 << ", "
	    << instruction.arg2 << ", " << instruction.result << ")";
	return out;
}

std::ostream &operator<<(std::ostream &out, const TAC &tac) {
	out << "Variables:\n";
	for (const auto &[_, value] : tac.variable_table) {
		out << value.type << " " << value.name;
		if (value.temporary)
			out << " (temporary)";
		out << "\n";
	}
	out << "\nLiterals:\n";
	for (const auto &literal : tac.literal_table) {
		out << literal.type << " " << literal.value << "\n";
	}
	out << "\n";

	int idx = 0;
	for (const auto &instruction : tac.instructions) {
		out << "(" << (idx++) << ") " << instruction << "\n";
	}
	return out;
}

TAC::Variable TAC::tempVar(const std::string &type) {
	Variable var{.name = "T" + std::to_string(++tempVariableCount),
	             .type = type,
	             .temporary = true};
	variable_table[var.name] = var;
	return var;
}

TAC::Variable TAC::lookupVar(const std::string &name, int pos) {
	auto var = variable_table.find(name);
	if (var == variable_table.end()) {
		throw CompileException(pos, "Unknown identifier: " + name);
	}
	return var->second;
}

static std::string typeOf(TAC::Value v) {
	if (std::holds_alternative<TAC::Variable>(v)) {
		return std::get<TAC::Variable>(v).type;
	} else if (std::holds_alternative<TAC::Literal>(v)) {
		return std::get<TAC::Literal>(v).type;
	} else {
		throw std::bad_variant_access();
	}
}

void TAC::generate(const std::string &op, Instruction::Arg arg1,
                   Instruction::Arg arg2, Instruction::Result result) {
	instructions.push_back({
	    .op = op,
	    .arg1 = arg1,
	    .arg2 = arg2,
	    .result = result,
	});
	nextQ++;
}

TAC::Literal TAC::makeLiteral(const std::string &value,
                              const std::string &type) {
	Literal literal{
	    .value = value,
	    .type = type,
	};
	literal_table.insert(literal);
	return literal;
}

TAC::TAC(const ProgramNode &ast) {
	translateVariableDeclaration(*ast.variables);
	translateStatements(*ast.statements);
}

void TAC::translateVariableDeclaration(const VariableDeclarationNode &node) {
	for (const auto &identifier : node.identifiers) {
		variable_table[identifier] = {
		    .name = identifier,
		    .type = node.type,
		    .temporary = false,
		};
	}
}

void TAC::translateStatements(const StatementsNode &node) {
	for (const auto &statement : node.statements) {
		translateStatement(*statement);
	}
}

void TAC::translateStatement(const StatementNode &node) {
	if (typeid(node) == typeid(AssignStatementNode)) {
		translateAssignStatement(
		    dynamic_cast<const AssignStatementNode &>(node));
	} else if (typeid(node) == typeid(IfStatementNode)) {
		translateIfStatement(dynamic_cast<const IfStatementNode &>(node));
	} else if (typeid(node) == typeid(DoWhileStatementNode)) {
		translateDoWhileStatement(
		    dynamic_cast<const DoWhileStatementNode &>(node));
	} else {
		throw std::runtime_error("Unknown statement");
	}
}

void TAC::translateAssignStatement(const AssignStatementNode &node) {
	auto variable = lookupVar(node.variable, node.position_begin);
	auto expression = translateExpression(*node.expression);
	if (typeOf(variable) != typeOf(expression)) {
		throw CompileException(
		    node.position_begin,
		    "Type mismatch in assignment: " + typeOf(variable) + " vs " +
		        typeOf(expression));
	}
	generate("=", expression, std::nullopt, variable);
}

void TAC::translateIfStatement(const IfStatementNode &node) {
	auto condition = translateCondition(*node.condition);
	if (typeOf(condition) != "bool") {
		throw CompileException(node.condition->position_begin,
		                       "If condition is not bool, actual: " +
		                           typeOf(condition));
	}
	Label trueExit{.num = nextQ + 2};
	Label falseExit{.num = -1};
	Label ifExit{.num = -1};
	generate("jnz", condition, std::nullopt, trueExit);
	int falseExitNo = nextQ;
	generate("j", std::nullopt, std::nullopt, falseExit);
	translateStatements(*node.true_action);
	int ifExitNo = nextQ;
	generate("j", std::nullopt, std::nullopt, ifExit);
	std::get<Label>(instructions[falseExitNo].result).num = nextQ;
	translateStatements(*node.false_action);
	std::get<Label>(instructions[ifExitNo].result).num = nextQ;
}

void TAC::translateDoWhileStatement(const DoWhileStatementNode &node) {
	Label loop{.num = nextQ};
	translateStatements(*node.loop_action);
	auto condition = translateCondition(*node.condition);
	if (typeOf(condition) != "bool") {
		throw CompileException(node.condition->position_begin,
		                       "Do-while condition is not bool, actual: " +
		                           typeOf(condition));
	}
	generate("jnz", condition, std::nullopt, loop);
}

TAC::Value TAC::translateExpression(const ExpressionNode &node) {
	auto x = translateItem(*node.items[0]);
	for (size_t i = 1; i < node.items.size(); i++) {
		auto y = translateItem(*node.items[i]);
		if (typeOf(x) != "string") {
			throw CompileException(node.items[0]->position_begin,
			                       "Concat operation requires string operands");
		}
		if (typeOf(y) != "string") {
			throw CompileException(node.items[i]->position_begin,
			                       "Concat operation requires string operands");
		}
		auto tmp = tempVar("string");
		generate("+", x, y, tmp);
		x = tmp;
	}
	return x;
}

TAC::Value TAC::translateCondition(const ConditionNode &node) {
	auto x = translateExpression(*node.lhs);
	auto y = translateExpression(*node.rhs);
	if (typeOf(x) != "string") {
		throw CompileException(node.lhs->position_begin,
		                       "Relation operator requires string operands");
	}
	if (typeOf(y) != "string") {
		throw CompileException(node.rhs->position_begin,
		                       "Relation operator requires string operands");
	}
	std::string op;
	switch (node.op) {
	case RelationOp::LESS:
		op = "<";
		break;
	case RelationOp::GREATER:
		op = ">";
		break;
	case RelationOp::LESS_EQUAL:
		op = "<=";
		break;
	case RelationOp::GREATER_EQUAL:
		op = ">=";
		break;
	case RelationOp::NOT_EQUAL:
		op = "!=";
		break;
	case RelationOp::EQUAL:
		op = "==";
		break;
	}
	auto tmp = tempVar("bool");
	generate(op, x, y, tmp);
	return tmp;
}

TAC::Value TAC::translateItem(const ItemNode &node) {
	auto x = translateFactor(*node.factor);
	for (auto repeat_time : node.repeat_times) {
		if (typeOf(x) != "string") {
			throw CompileException(node.factor->position_begin,
			                       "Repeat operator requires string operands");
		}
		auto tmp = tempVar("string");
		auto arg2 = makeLiteral(std::to_string(repeat_time), "int");
		generate("*", x, arg2, tmp);
		x = tmp;
	}
	return x;
}

TAC::Value TAC::translateFactor(const FactorNode &node) {
	if (typeid(node) == typeid(StringFactorNode)) {
		return translateStringFactor(
		    dynamic_cast<const StringFactorNode &>(node));
	} else if (typeid(node) == typeid(VariableFactorNode)) {
		return translateVariableFactor(
		    dynamic_cast<const VariableFactorNode &>(node));
	} else if (typeid(node) == typeid(ExpressionFactorNode)) {
		return translateExpressionFactor(
		    dynamic_cast<const ExpressionFactorNode &>(node));
	} else {
		throw std::runtime_error("Unknown factor");
	}
}

TAC::Value TAC::translateStringFactor(const StringFactorNode &node) {
	return makeLiteral(node.str, "string");
}

TAC::Value TAC::translateVariableFactor(const VariableFactorNode &node) {
	return lookupVar(node.identifier, node.position_begin);
}

TAC::Value TAC::translateExpressionFactor(const ExpressionFactorNode &node) {
	return translateExpression(*node.expression);
}
