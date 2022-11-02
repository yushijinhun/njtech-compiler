#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

class ASTNode {
  public:
	int position_begin;
	int position_end;

	virtual ~ASTNode() = default;
	virtual void print_json(std::ostream &out) const = 0;
	friend std::ostream &operator<<(std::ostream &out, const ASTNode &ast);
};

class FactorNode : public ASTNode {};

class StringFactorNode : public FactorNode {
  public:
	std::string str;
	void print_json(std::ostream &out) const;
};

class VariableFactorNode : public FactorNode {
  public:
	std::string identifier;
	void print_json(std::ostream &out) const;
};

class ItemNode : public ASTNode {
  public:
	std::unique_ptr<FactorNode> factor;
	std::vector<int> repeat_times;
	void print_json(std::ostream &out) const;
};

class ExpressionNode : public ASTNode {
  public:
	std::vector<std::unique_ptr<ItemNode>> items;
	void print_json(std::ostream &out) const;
};

class ExpressionFactorNode : public FactorNode {
  public:
	std::unique_ptr<ExpressionNode> expression;
	void print_json(std::ostream &out) const;
};

enum class RelationOp {
	LESS,
	GREATER,
	LESS_EQUAL,
	GREATER_EQUAL,
	NOT_EQUAL,
	EQUAL
};

class ConditionNode : public ASTNode {
  public:
	RelationOp op;
	std::unique_ptr<ExpressionNode> lhs;
	std::unique_ptr<ExpressionNode> rhs;
	void print_json(std::ostream &out) const;
};

class StatementNode : public ASTNode {};

class StatementsNode : public ASTNode {
  public:
	std::vector<std::unique_ptr<StatementNode>> statements;
	void print_json(std::ostream &out) const;
};

class AssignStatementNode : public StatementNode {
  public:
	std::string variable;
	std::unique_ptr<ExpressionNode> expression;
	void print_json(std::ostream &out) const;
};

class IfStatementNode : public StatementNode {
  public:
	std::unique_ptr<ConditionNode> condition;
	std::unique_ptr<StatementsNode> true_action;
	std::unique_ptr<StatementsNode> false_action;
	void print_json(std::ostream &out) const;
};

class DoWhileStatementNode : public StatementNode {
  public:
	std::unique_ptr<ConditionNode> condition;
	std::unique_ptr<StatementsNode> loop_action;
	void print_json(std::ostream &out) const;
};

class VariableDeclarationNode : public ASTNode {
  public:
	std::string type;
	std::vector<std::string> identifiers;
	void print_json(std::ostream &out) const;
};

class ProgramNode : public ASTNode {
  public:
	std::unique_ptr<VariableDeclarationNode> variables;
	std::unique_ptr<StatementsNode> statements;
	void print_json(std::ostream &out) const;
};
