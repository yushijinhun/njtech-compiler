#include "ast.hpp"

std::ostream &operator<<(std::ostream &out, const ASTNode &ast) {
	ast.print_json(out);
	return out;
}

void ProgramNode::print_json(std::ostream &out) const {
	out << R"({"variables":)" << *variables << R"(,"statements":)"
	    << *statements << R"(})";
}

void VariableDeclarationNode::print_json(std::ostream &out) const {
	out << R"({"type":"string","identifiers":[)";
	bool first = true;
	for (const auto &identifier : identifiers) {
		if (first) {
			first = false;
		} else {
			out << ",";
		}
		out << '"' << identifier << '"';
	}
	out << R"(]})";
}

void StatementsNode::print_json(std::ostream &out) const {
	out << '[';
	bool first = true;
	for (const auto &statement : statements) {
		if (first) {
			first = false;
		} else {
			out << ",";
		}
		out << *statement;
	}
	out << ']';
}

void AssignStatementNode::print_json(std::ostream &out) const {
	out << R"({"type":"assign","variable":")" << variable
	    << R"(","expression":)" << *expression << R"(})";
}

void IfStatementNode::print_json(std::ostream &out) const {
	out << R"({"type":"if","condition":)" << *condition << R"(,"true_action":)"
	    << *true_action << R"(,"false_action":)" << *false_action << R"(})";
}

void DoWhileStatementNode::print_json(std::ostream &out) const {
	out << R"({"type":"do_while","condition":)" << *condition
	    << R"(,"loop_action":)" << *loop_action << R"(})";
}

void ConditionNode::print_json(std::ostream &out) const {
	out << R"({"op":")";
	switch (op) {
	case RelationOp::LESS:
		out << "less";
		break;
	case RelationOp::GREATER:
		out << "greater";
		break;
	case RelationOp::LESS_EQUAL:
		out << "less_equal";
		break;
	case RelationOp::GREATER_EQUAL:
		out << "greater_equal";
		break;
	case RelationOp::NOT_EQUAL:
		out << "not_equal";
		break;
	case RelationOp::EQUAL:
		out << "equal";
		break;
	}
	out << R"(","lhs":)" << *lhs << R"(,"rhs":)" << *rhs << R"(})";
}

void ExpressionNode::print_json(std::ostream &out) const {
	out << '[';
	bool first = true;
	for (const auto &item : items) {
		if (first) {
			first = false;
		} else {
			out << ",";
		}
		out << *item;
	}
	out << ']';
}

void ItemNode::print_json(std::ostream &out) const {
	out << R"({"factor":)" << *factor << R"(,"repeat_times":[)";
	bool first = true;
	for (const auto &repeat_time : repeat_times) {
		if (first) {
			first = false;
		} else {
			out << ",";
		}
		out << repeat_time;
	}
	out << R"(]})";
}

void StringFactorNode::print_json(std::ostream &out) const {
	out << R"({"type":"string","value":")" << str << R"("})";
}

void VariableFactorNode::print_json(std::ostream &out) const {
	out << R"({"type":"variable","identifier":")" << identifier << R"("})";
}

void ExpressionFactorNode::print_json(std::ostream &out) const {
	out << R"({"type":"expression","expression":)" << *expression << R"(})";
}
