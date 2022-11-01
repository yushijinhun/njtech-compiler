#pragma once

#include <memory>
#include <string>
#include <vector>

class VariableDeclarationNode;
class StatementsNode;
class StatementNode;
class ExpressionNode;
class ConditionNode;
class ItemNode;
class FactorNode;

class ProgramNode {
  public:
	std::unique_ptr<VariableDeclarationNode> variables;
	std::unique_ptr<StatementsNode> statements;
};

class VariableDeclarationNode {
  public:
	std::string type;
	std::vector<std::string> identifiers;
};

class StatementsNode {
  public:
	std::vector<std::unique_ptr<StatementNode>> statements;
};

class StatementNode {};

class AssignStatementNode : public StatementNode {
  public:
	std::string variable;
	std::unique_ptr<ExpressionNode> expression;
};

class IfStatementNode : public StatementNode {
  public:
	std::unique_ptr<ConditionNode> condition;
	std::unique_ptr<StatementsNode> true_action;
	std::unique_ptr<StatementsNode> false_action;
};

class WhileStatementNode : public StatementNode {
  public:
	std::unique_ptr<ConditionNode> condition;
	std::unique_ptr<StatementsNode> loop_action;
};

class ExpressionNode {
  public:
	std::vector<std::unique_ptr<ItemNode>> items;
};

class ItemNode {
  public:
	std::unique_ptr<FactorNode> factor;
	std::vector<int> repeat_times;
};

class FactorNode {};

class StringFactorNode : public FactorNode {
  public:
	std::string str;
};

class IdentifierFactorNode : public FactorNode {
  public:
	std::string identifier;
};

class ExpressionFactorNode : public FactorNode {
  public:
	std::unique_ptr<ExpressionNode> expression;
};

enum class RelationOp {
	LESS,
	GREATER,
	LESS_EQUAL,
	GREATER_EQUAL,
	NOT_EQUAL,
	EQUAL
};

class ConditionNode {
  public:
	std::unique_ptr<ExpressionNode> lhs;
	RelationOp op;
	std::unique_ptr<ExpressionNode> rhs;
};
