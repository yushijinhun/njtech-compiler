#include "codegen.hpp"
#include "error.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>

namespace compiler {

llvm::Value *LLVMCodeGen::genStrlen(llvm::Value *str_ptr) {
	// ---- LLVM IR ----
	// entry:
	//   ...
	//   br label %loop
	// loop:
	//   %idx = phi i32 [ 0, %entry ], [ %next_idx, %loop ]
	//   %addr = getelementptr inbounds i8, ptr %str_ptr, i32 %idx
	//   %val = load i8, ptr %addr
	//   %cond = icmp eq i8 %val, 0
	//   %next_idx = add i32 %idx, 1
	//   br i1 %cond, label %cont, label %loop
	// cont:
	//   ... (ret i32 %idx)
	auto *entry = builder.GetInsertBlock();
	auto *current_func = entry->getParent();
	auto *loop = llvm::BasicBlock::Create(ctx, "_strlen_loop", current_func);
	builder.CreateBr(loop);
	builder.SetInsertPoint(loop);
	auto *idx = builder.CreatePHI(builder.getInt32Ty(), 2, "_strlen_idx");
	idx->addIncoming(builder.getInt32(0), entry);
	auto *addr = builder.CreateInBoundsGEP(builder.getInt8Ty(), str_ptr, idx,
	                                       "_strlen_addr");
	auto *val = builder.CreateLoad(builder.getInt8Ty(), addr, "_strlen_val");
	auto *cond = builder.CreateICmpEQ(val, builder.getInt8(0), "_strlen_cond");
	auto *next_idx =
	    builder.CreateAdd(idx, builder.getInt32(1), "_strlen_next_idx");
	idx->addIncoming(next_idx, loop);
	auto *cont = llvm::BasicBlock::Create(ctx, "_strlen_cont", current_func);
	builder.CreateCondBr(cond, cont, loop);
	builder.SetInsertPoint(cont);
	return idx;
}

llvm::Value *LLVMCodeGen::genStrAlloc(llvm::Value *len) {
	auto *size = builder.CreateAdd(len, builder.getInt32(1), "_stralloc_size");
	auto *ptr = llvm::CallInst::CreateMalloc(
	    builder.GetInsertBlock(), builder.getInt32Ty(), builder.getInt8Ty(),
	    size, nullptr, nullptr, "_stralloc_ptr");
	builder.Insert(ptr);
	return ptr;
}

void LLVMCodeGen::genStrFree(llvm::Value *ptr) {
	builder.Insert(llvm::CallInst::CreateFree(ptr, builder.GetInsertBlock()));
}

void LLVMCodeGen::destructTransientValue(DestructibleValue &&val) {
	if (!val.transient || !val.val->getType()->isPointerTy()) {
		return;
	}
	genStrFree(val.val);
}

void LLVMCodeGen::visitVariableDeclaration(
    const VariableDeclarationNode &node) {
	llvm::Type *type;
	if (node.type == "string") {
		type = builder.getInt8PtrTy();
	} else {
		throw CompileException(node.position_begin,
		                       "Unsupported variable type: " + node.type);
	}
	for (const auto &name : node.identifiers) {
		if (variables.contains(name)) {
			throw CompileException(node.position_begin,
			                       "Variable is already defined: " + name);
		}
		auto *ptr = builder.CreateAlloca(type, nullptr, name);
		if (type->isPointerTy()) {
			// initialize pointers as null
			builder.CreateStore(llvm::ConstantPointerNull::get(
			                        llvm::dyn_cast<llvm::PointerType>(type)),
			                    ptr);
		}
		variables[name] = ptr;
	}
}

LLVMCodeGen::DestructibleValue
LLVMCodeGen::visitStringFactor(const StringFactorNode &node) {
	return {
	    .val = builder.CreateGlobalStringPtr(node.str),
	    .transient = false,
	    .strlen = builder.getInt32(node.str.size()),
	};
}

LLVMCodeGen::DestructibleValue
LLVMCodeGen::visitVariableFactor(const VariableFactorNode &node) {
	auto it = variables.find(node.identifier);
	if (it == variables.end()) {
		throw CompileException(node.position_begin,
		                       "Undefined variable: " + node.identifier);
	}
	auto *var_ptr = it->second;
	return {
	    .val = builder.CreateLoad(var_ptr->getAllocatedType(), var_ptr,
	                              node.identifier),
	    .transient = false,
	    .strlen = std::nullopt,
	};
}

LLVMCodeGen::DestructibleValue
LLVMCodeGen::visitExpressionFactor(const ExpressionFactorNode &node) {
	return visitExpression(*node.expression);
}

LLVMCodeGen::DestructibleValue
LLVMCodeGen::visitFactor(const FactorNode &node) {
	if (typeid(node) == typeid(StringFactorNode)) {
		return visitStringFactor(dynamic_cast<const StringFactorNode &>(node));
	} else if (typeid(node) == typeid(VariableFactorNode)) {
		return visitVariableFactor(
		    dynamic_cast<const VariableFactorNode &>(node));
	} else if (typeid(node) == typeid(ExpressionFactorNode)) {
		return visitExpressionFactor(
		    dynamic_cast<const ExpressionFactorNode &>(node));
	} else {
		throw std::runtime_error("Unknown factor");
	}
}

LLVMCodeGen::DestructibleValue LLVMCodeGen::visitItem(const ItemNode &node) {
	auto factor = visitFactor(*node.factor);
	if (node.repeat_times.empty()) {
		return factor;
	}
	auto *type = factor.val->getType();
	if (!type->isPointerTy()) {
		throw CompileException(
		    node.position_begin,
		    "Left operand of repeat operator must be string");
	}
	llvm::Value *len;
	if (factor.strlen.has_value()) {
		// optimization: reuse strlen
		len = *factor.strlen;
	} else {
		len = genStrlen(factor.val);
	}
	for (auto repeat_time : node.repeat_times) {
		if (repeat_time < 0) {
			throw CompileException(node.position_begin,
			                       "Repeat times can't be negative");
		}
		auto *times = builder.getInt32(repeat_time);
		auto *newlen = builder.CreateMul(len, times, "_repeat_newlen");
		auto *result = genStrAlloc(newlen);

		// String repeat code generation
		{
			// ---- C code ----
			// result[newlen] = 0;
			// int idx = 0;
			// for (int i = 0; i < times; i++) {
			//   for (int j = 0; j < len; j++) {
			//     result[idx] = factor[j];
			//     idx++;
			//   }
			// }
			// ---- LLVM IR ----
			// entry:
			//   ...
			//   %lastaddr = getelementptr inbounds i8, ptr %result, i32 %newlen
			//   store i8 0, ptr %lastaddr
			//   %times_is_zero = icmp eq i32 %times, 0
			//   br i1 %times_is_zero, label %cont, label %outer_pre
			// outer_pre: ; preds = %entry
			//   %len_is_zero = icmp eq i32 %len, 0
			//   br label %inner_pre
			// outer_loop: ; preds = %inner_loop, %inner_pre
			//   %dstidx_2 = phi i32 [ %dstidx_1, %inner_pre ], [ %next_dstidx, %inner_loop ]
			//   %next_outer_i = add i32 %outer_i, 1
			//   %outer_finished = icmp eq i32 %next_outer_i, %times
			//   br i1 %outer_finished, label %cont, label %inner_pre
			// inner_pre: ; preds = %outer_pre, %outer_loop
			//   %outer_i = phi i32 [ 0, %outer_pre ], [ %next_outer_i, %outer_loop ]
			//   %dstidx_1 = phi i32 [ 0, %outer_pre ], [ %dstidx_2, %outer_loop ]
			//   br i1 %len_is_zero, label %outer_loop, label %inner_loop
			// inner_loop: ; preds = %inner_pre, %inner_loop
			//   %srcidx = phi i32 [ %next_srcidx, %inner_loop ], [ 0, %inner_pre ]
			//   %dstidx = phi i32 [ %next_dstidx, %inner_loop ], [ %dstidx_1, %inner_pre ]
			//   %srcaddr = getelementptr inbounds i8, ptr %factor, i32 %srcidx
			//   %src = load i8, ptr %srcaddr
			//   %dstaddr = getelementptr inbounds i8, ptr %result, i32 %dstidx
			//   store i8 %src, ptr %dstaddr
			//   %next_dstidx = add i32 %dstidx, 1
			//   %next_srcidx = add i32 %srcidx, 1
			//   %inner_finished = icmp eq i32 %next_srcidx, %len
			//   br i1 %inner_finished, label %outer_loop, label %inner_loop
			// cont: ; preds = %outer_loop, %entry
			//   ...

			auto *entry = builder.GetInsertBlock();
			auto *current_func = entry->getParent();
			auto *outer_pre = llvm::BasicBlock::Create(ctx, "_repeat_outer_pre",
			                                           current_func);
			auto *outer_loop = llvm::BasicBlock::Create(
			    ctx, "_repeat_outer_loop", current_func);
			auto *inner_pre = llvm::BasicBlock::Create(ctx, "_repeat_inner_pre",
			                                           current_func);
			auto *inner_loop = llvm::BasicBlock::Create(
			    ctx, "_repeat_inner_loop", current_func);
			auto *cont =
			    llvm::BasicBlock::Create(ctx, "_repeat_cont", current_func);
			auto *lastaddr = builder.CreateInBoundsGEP(
			    builder.getInt8Ty(), result, newlen, "_repeat_lastaddr");
			builder.CreateStore(builder.getInt8(0), lastaddr);
			auto *times_is_zero = builder.CreateICmpEQ(
			    times, builder.getInt32(0), "_repeat_times_is_zero");
			builder.CreateCondBr(times_is_zero, cont, outer_pre);

			builder.SetInsertPoint(outer_pre);
			auto *len_is_zero = builder.CreateICmpEQ(len, builder.getInt32(0),
			                                         "_repeat_len_is_zero");
			builder.CreateBr(inner_pre);

			builder.SetInsertPoint(inner_pre);
			auto *outer_i =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_repeat_outer_i");
			auto *dstidx_1 =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_repeat_dstidx_1");
			builder.CreateCondBr(len_is_zero, outer_loop, inner_loop);

			builder.SetInsertPoint(outer_loop);
			auto *dstidx_2 =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_repeat_dstidx_2");
			auto *next_outer_i = builder.CreateAdd(outer_i, builder.getInt32(1),
			                                       "_repeat_next_outer_i");
			auto *outer_finished = builder.CreateICmpEQ(
			    next_outer_i, times, "_repeat_outer_finished");
			builder.CreateCondBr(outer_finished, cont, inner_pre);

			builder.SetInsertPoint(inner_loop);
			auto *srcidx =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_repeat_srcidx");
			auto *dstidx =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_repeat_dstidx");
			auto *srcaddr = builder.CreateInBoundsGEP(
			    builder.getInt8Ty(), factor.val, srcidx, "_repeat_srcaddr");
			auto *src =
			    builder.CreateLoad(builder.getInt8Ty(), srcaddr, "_repeat_src");
			auto *dstaddr = builder.CreateInBoundsGEP(
			    builder.getInt8Ty(), result, dstidx, "_repeat_dstaddr");
			builder.CreateStore(src, dstaddr);
			auto *next_dstidx = builder.CreateAdd(dstidx, builder.getInt32(1),
			                                      "_repeat_next_dst_idx");
			auto *next_srcidx = builder.CreateAdd(srcidx, builder.getInt32(1),
			                                      "_repeat_next_src_idx");
			auto *inner_finished = builder.CreateICmpEQ(
			    next_srcidx, len, "_repeat_inner_finished");
			builder.CreateCondBr(inner_finished, outer_loop, inner_loop);

			dstidx_2->addIncoming(dstidx_1, inner_pre);
			dstidx_2->addIncoming(next_dstidx, inner_loop);
			outer_i->addIncoming(builder.getInt32(0), outer_pre);
			outer_i->addIncoming(next_outer_i, outer_loop);
			dstidx_1->addIncoming(builder.getInt32(0), outer_pre);
			dstidx_1->addIncoming(dstidx_2, outer_loop);
			srcidx->addIncoming(next_srcidx, inner_loop);
			srcidx->addIncoming(builder.getInt32(0), inner_pre);
			dstidx->addIncoming(next_dstidx, inner_loop);
			dstidx->addIncoming(dstidx_1, inner_pre);

			builder.SetInsertPoint(cont);
		}

		destructTransientValue(std::move(factor));
		factor = {
		    .val = result,
		    .transient = true,
		    .strlen = newlen,
		};
		len = newlen;
	}
	return factor;
}

LLVMCodeGen::DestructibleValue
LLVMCodeGen::visitExpression(const ExpressionNode &node) {
	auto item_count = node.items.size();
	if (item_count == 0) {
		throw CompileException(node.position_begin,
		                       "Expression can't be empty");
	}
	if (item_count == 1) {
		return visitItem(*node.items[0]);
	}
	llvm::Value *total_len = nullptr;
	std::vector<DestructibleValue> item_vals;
	for (auto &item_node : node.items) {
		auto item = visitItem(*item_node);
		if (!item.val->getType()->isPointerTy()) {
			throw CompileException(item_node->position_begin,
			                       "Operand of concat operator must be string");
		}
		if (!item.strlen.has_value()) {
			item.strlen = genStrlen(item.val);
		}
		if (total_len == nullptr) {
			total_len = *item.strlen;
		} else {
			total_len =
			    builder.CreateAdd(total_len, *item.strlen, "_concat_tmplen");
		}
		item_vals.push_back(std::move(item));
	}
	auto *result = genStrAlloc(total_len);

	// String concat code generation
	{
		// ---- C Code ----
		// for (size_t idx = 0; idx < len; idx++) {
		//   result[offset++] = src[idx];
		// }
		// ---- LLVM IR ----
		// entry:
		//   ...
		//   %len_is_zero = icmp eq i32 %len, 0
		//   br i1 %len_is_zero, label %cont, label %loop
		// loop: ; preds = %entry, %loop
		//   %srcidx = phi i32 [ %next_srcidx, %loop ], [ 0, %entry ]
		//   %dstidx = phi i32 [ %next_dstidx, %loop ], [ %offset, %entry ]
		//   %srcptr = getelementptr inbounds i8, ptr %src, i32 %srcidx
		//   %src_element = load i8, ptr %srcptr
		//   %dstptr = getelementptr inbounds i8, ptr %result, i32 %dstidx
		//   store i8 %src_element, ptr %dstptr
		//   %next_srcidx = add i32 %srcidx, 1
		//   %next_dstidx = add i32 %dstidx, 1
		//   %cond = icmp eq i32 %next_srcidx, %len
		//   br i1 %cond, label %cont, label %loop
		// cont: ; preds = %loop, %entry
		//   %end_idx = phi i32 [ %offset, %entry ], [ %next_dstidx, %loop ]
		//   ...
		auto *lastaddr = builder.CreateInBoundsGEP(
		    builder.getInt8Ty(), result, total_len, "_concat_lastaddr");
		builder.CreateStore(builder.getInt8(0), lastaddr);
		llvm::Value *offset = builder.getInt32(0);
		for (auto &item_val : item_vals) {
			auto *len = *item_val.strlen;
			auto *src = item_val.val;
			auto *entry = builder.GetInsertBlock();
			auto *current_func = entry->getParent();
			auto *loop =
			    llvm::BasicBlock::Create(ctx, "_concat_loop", current_func);
			auto *cont =
			    llvm::BasicBlock::Create(ctx, "_concat_cont", current_func);
			auto *len_is_zero = builder.CreateICmpEQ(len, builder.getInt32(0),
			                                         "_concat_len_is_zero");
			builder.CreateCondBr(len_is_zero, cont, loop);

			builder.SetInsertPoint(loop);
			auto *srcidx =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_concat_srcidx");
			auto *dstidx =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_concat_dstidx");
			auto *srcptr = builder.CreateInBoundsGEP(builder.getInt8Ty(), src,
			                                         srcidx, "_concat_srcptr");
			auto *src_element = builder.CreateLoad(builder.getInt8Ty(), srcptr,
			                                       "_concat_src_element");
			auto *dstptr = builder.CreateInBoundsGEP(
			    builder.getInt8Ty(), result, dstidx, "_concat_dstptr");
			builder.CreateStore(src_element, dstptr);
			auto *next_srcidx = builder.CreateAdd(srcidx, builder.getInt32(1),
			                                      "_concat_next_srcidx");
			auto *next_dstidx = builder.CreateAdd(dstidx, builder.getInt32(1),
			                                      "_concat_next_dstidx");
			auto *cond = builder.CreateICmpEQ(next_srcidx, len, "_concat_cond");
			builder.CreateCondBr(cond, cont, loop);

			builder.SetInsertPoint(cont);
			auto *end_idx = builder.CreatePHI(builder.getInt32Ty(), 2);

			srcidx->addIncoming(next_srcidx, loop);
			srcidx->addIncoming(builder.getInt32(0), entry);
			dstidx->addIncoming(next_dstidx, loop);
			dstidx->addIncoming(offset, entry);
			end_idx->addIncoming(offset, entry);
			end_idx->addIncoming(next_dstidx, loop);

			destructTransientValue(std::move(item_val));

			offset = end_idx;
		}
	}

	return {
	    .val = result,
	    .transient = true,
	    .strlen = total_len,
	};
}

llvm::Value *LLVMCodeGen::visitCondition(const ConditionNode &node) {
	auto lhs = visitExpression(*node.lhs);
	if (!lhs.val->getType()->isPointerTy()) {
		throw new CompileException(
		    node.lhs->position_begin,
		    "Operand of relation operator must be string");
	}
	if (!lhs.strlen.has_value()) {
		lhs.strlen = genStrlen(lhs.val);
	}

	switch (node.op) {

	case RelationOp::EQUAL:
	case RelationOp::NOT_EQUAL: {
		auto rhs = visitExpression(*node.rhs);
		if (!rhs.val->getType()->isPointerTy()) {
			throw new CompileException(
			    node.rhs->position_begin,
			    "Operand of relation operator must be string");
		}
		if (!rhs.strlen.has_value()) {
			rhs.strlen = genStrlen(rhs.val);
		}

		llvm::PHINode *result;
		// String equal compare code generation
		{
			auto *a = lhs.val;
			auto *b = rhs.val;
			auto *len_a = *lhs.strlen;
			auto *len_b = *rhs.strlen;
			// ---- C Code ----
			// bool eq(char* a, char* b, size_t len_a, size_t len_b) {
			//   if (len_a != len_b) {
			//     return false;
			//   }
			//   for (size_t i = 0; i < len_a; i++) {
			//     if (a[i] != b[i]) {
			//       return false;
			//     }
			//   }
			//   return true;
			// }
			// ---- LLVM IR ----
			// entry:
			//   ...
			//   %samelen = icmp eq i32 %len_a, %len_b
			//   br i1 %samelen, label %check_empty, label %cont
			// check_empty: ; preds = %entry
			//   %empty = icmp eq i32 %len_a, 0
			//   br i1 %empty, label %cont, label %check_first
			// check_first: ; preds = %check_empty
			//   %first_a = load i8, ptr %a
			//   %first_b = load i8, ptr %b
			//   %samefirst = icmp eq i8 %first_a, %first_b
			//   br i1 %samefirst, label %loop_increment, label %cont
			// loop_increment: ; preds = %check_first, %loop_body
			//   %old_idx = phi i32 [ %idx, %loop_body ], [ 0, %check_first ]
			//   %idx = add i32 %old_idx, 1
			//   %loop_finished = icmp eq i32 %idx, %len_a
			//   br i1 %loop_finished, label %loop_end, label %loop_body
			// loop_body: ; preds = %loop_increment
			//   %addr_a = getelementptr inbounds i8, ptr %a, i32 %idx
			//   %val_a = load i8, ptr %addr_a
			//   %addr_b = getelementptr inbounds i8, ptr %b, i32 %idx
			//   %val_b = load i8, ptr %addr_b
			//   %sameval = icmp eq i8 %val_a, %val_b
			//   br i1 %sameval, label %loop_increment, label %loop_end
			// loop_end: ; preds = %loop_increment, %loop_body
			//   %streq = icmp uge i32 %idx, %len_a
			//   br label %cont
			// cont: ; preds = %check_empty, %check_first, %loop_end, %entry
			//   result = phi i1 [ false, %entry ], [ true, %check_empty ], [ false, %check_first ], [ %streq, %loop_end ]
			//   ...
			auto *entry = builder.GetInsertBlock();
			auto *current_func = entry->getParent();
			auto *check_empty = llvm::BasicBlock::Create(
			    ctx, "_streq_check_empty", current_func);
			auto *check_first = llvm::BasicBlock::Create(
			    ctx, "_streq_check_first", current_func);
			auto *loop_increment = llvm::BasicBlock::Create(
			    ctx, "_streq_loop_increment", current_func);
			auto *loop_body =
			    llvm::BasicBlock::Create(ctx, "_streq_loop_body", current_func);
			auto *loop_end =
			    llvm::BasicBlock::Create(ctx, "_streq_loop_end", current_func);
			auto *cont =
			    llvm::BasicBlock::Create(ctx, "_streq_cont", current_func);
			auto *samelen =
			    builder.CreateICmpEQ(len_a, len_b, "_streq_samelen");
			builder.CreateCondBr(samelen, check_empty, cont);

			builder.SetInsertPoint(check_empty);
			auto *empty = builder.CreateICmpEQ(len_a, builder.getInt32(0),
			                                   "_streq_empty");
			builder.CreateCondBr(empty, cont, check_first);

			builder.SetInsertPoint(check_first);
			auto *first_a =
			    builder.CreateLoad(builder.getInt8Ty(), a, "_streq_first_a");
			auto *first_b =
			    builder.CreateLoad(builder.getInt8Ty(), b, "_streq_first_b");
			auto *samefirst =
			    builder.CreateICmpEQ(first_a, first_b, "_streq_samefirst");
			builder.CreateCondBr(samefirst, loop_increment, cont);

			builder.SetInsertPoint(loop_increment);
			auto *old_idx =
			    builder.CreatePHI(builder.getInt32Ty(), 2, "_streq_old_idx");
			auto *idx =
			    builder.CreateAdd(old_idx, builder.getInt32(1), "_streq_idx");
			auto *loop_finished =
			    builder.CreateICmpEQ(idx, len_a, "_streq_loop_finished");
			builder.CreateCondBr(loop_finished, loop_end, loop_body);

			builder.SetInsertPoint(loop_body);
			auto *addr_a = builder.CreateInBoundsGEP(builder.getInt8Ty(), a,
			                                         idx, "_streq_addr_a");
			auto *val_a =
			    builder.CreateLoad(builder.getInt8Ty(), addr_a, "_streq_val_a");
			auto *addr_b = builder.CreateInBoundsGEP(builder.getInt8Ty(), b,
			                                         idx, "_streq_addr_b");
			auto *val_b =
			    builder.CreateLoad(builder.getInt8Ty(), addr_b, "_streq_val_b");
			auto *sameval =
			    builder.CreateICmpEQ(val_a, val_b, "_streq_sameval");
			builder.CreateCondBr(sameval, loop_increment, loop_end);

			builder.SetInsertPoint(loop_end);
			auto *streq = builder.CreateICmpEQ(idx, len_a, "_streq_streq");
			builder.CreateBr(cont);

			builder.SetInsertPoint(cont);
			result = builder.CreatePHI(builder.getInt1Ty(), 4);

			old_idx->addIncoming(idx, loop_body);
			old_idx->addIncoming(builder.getInt32(0), check_first);
			result->addIncoming(builder.getFalse(), entry);
			result->addIncoming(builder.getTrue(), check_empty);
			result->addIncoming(builder.getFalse(), check_first);
			result->addIncoming(streq, loop_end);
		}
		destructTransientValue(std::move(lhs));
		destructTransientValue(std::move(rhs));
		switch (node.op) {
		case RelationOp::EQUAL:
			return result;
		case RelationOp::NOT_EQUAL:
			return builder.CreateNot(result, "_streq_not");
		default:
			throw std::runtime_error("Unknown op");
		}
	}

	case RelationOp::LESS:
	case RelationOp::GREATER:
	case RelationOp::LESS_EQUAL:
	case RelationOp::GREATER_EQUAL: {
		auto *lhs_len = *lhs.strlen;
		destructTransientValue(std::move(lhs));
		auto rhs = visitExpression(*node.rhs);
		if (!rhs.val->getType()->isPointerTy()) {
			throw new CompileException(
			    node.rhs->position_begin,
			    "Operand of relation operator must be string");
		}
		if (!rhs.strlen.has_value()) {
			rhs.strlen = genStrlen(rhs.val);
		}
		auto *rhs_len = *rhs.strlen;
		destructTransientValue(std::move(rhs));

		switch (node.op) {
		case RelationOp::LESS:
			return builder.CreateICmpULT(lhs_len, rhs_len, "_cond");
		case RelationOp::GREATER:
			return builder.CreateICmpUGT(lhs_len, rhs_len, "_cond");
		case RelationOp::LESS_EQUAL:
			return builder.CreateICmpULE(lhs_len, rhs_len, "_cond");
		case RelationOp::GREATER_EQUAL:
			return builder.CreateICmpUGE(lhs_len, rhs_len, "_cond");
		default:
			throw std::runtime_error("Unknown op");
		}
	}

	default:
		throw std::runtime_error("Unknown op");
	}
}

void LLVMCodeGen::visitAssignStatement(const AssignStatementNode &node) {
	auto var_it = variables.find(node.variable);
	if (var_it == variables.end()) {
		throw CompileException(node.position_begin,
		                       "Undefined variable: " + node.variable);
	}
	auto *var_ptr = var_it->second;
	auto expr = visitExpression(*node.expression);
	if (!var_ptr->getAllocatedType()->isPointerTy() ||
	    !expr.val->getType()->isPointerTy()) {
		throw CompileException(node.position_begin,
		                       "Assignment requires string operands");
	}

	// Destruct old string (freeing a nullptr is safe)
	auto *oldstr =
	    builder.CreateLoad(builder.getInt8PtrTy(), var_ptr, "_assign_oldstr");
	genStrFree(oldstr);

	if (expr.transient) {
		// Move
		builder.CreateStore(expr.val, var_ptr);
	} else {
		// Copy
		if (!expr.strlen.has_value()) {
			expr.strlen = genStrlen(expr.val);
		}
		auto *src = expr.val;
		auto *strlen = *expr.strlen;
		auto *size =
		    builder.CreateAdd(strlen, builder.getInt32(1), "_assign_size");
		auto *dst = llvm::CallInst::CreateMalloc(
		    builder.GetInsertBlock(), builder.getInt32Ty(), builder.getInt8Ty(),
		    size, nullptr, nullptr, "_assign_dst");
		builder.Insert(dst);
		builder.CreateMemCpy(dst, llvm::Align(), src, llvm::Align(), size);
		builder.CreateStore(dst, var_ptr);
	}
}

void LLVMCodeGen::visitIfStatement(const IfStatementNode &node) {
	auto *entry = builder.GetInsertBlock();
	auto *current_func = entry->getParent();
	auto *true_block = llvm::BasicBlock::Create(ctx, "if_true", current_func);
	auto *false_block = llvm::BasicBlock::Create(ctx, "if_false", current_func);
	auto *cont_block = llvm::BasicBlock::Create(ctx, "if_cont", current_func);
	auto *cond = visitCondition(*node.condition);
	builder.CreateCondBr(cond, true_block, false_block);

	builder.SetInsertPoint(true_block);
	visitStatements(*node.true_action);
	builder.CreateBr(cont_block);

	builder.SetInsertPoint(false_block);
	visitStatements(*node.false_action);
	builder.CreateBr(cont_block);

	builder.SetInsertPoint(cont_block);
}

void LLVMCodeGen::visitDoWhileStatement(const DoWhileStatementNode &node) {
	auto *entry = builder.GetInsertBlock();
	auto *current_func = entry->getParent();
	auto *loop_block =
	    llvm::BasicBlock::Create(ctx, "dowhile_loop", current_func);
	auto *cont_block =
	    llvm::BasicBlock::Create(ctx, "dowhile_cont", current_func);
	builder.CreateBr(loop_block);

	builder.SetInsertPoint(loop_block);
	visitStatements(*node.loop_action);
	auto *cond = visitCondition(*node.condition);
	builder.CreateCondBr(cond, loop_block, cont_block);

	builder.SetInsertPoint(cont_block);
}

void LLVMCodeGen::visitStatement(const StatementNode &node) {
	if (typeid(node) == typeid(AssignStatementNode)) {
		visitAssignStatement(dynamic_cast<const AssignStatementNode &>(node));
	} else if (typeid(node) == typeid(IfStatementNode)) {
		visitIfStatement(dynamic_cast<const IfStatementNode &>(node));
	} else if (typeid(node) == typeid(DoWhileStatementNode)) {
		visitDoWhileStatement(dynamic_cast<const DoWhileStatementNode &>(node));
	} else {
		throw std::runtime_error("Unknown statement");
	}
}

void LLVMCodeGen::visitStatements(const StatementsNode &node) {
	for (auto &statement : node.statements) {
		visitStatement(*statement);
	}
}

void LLVMCodeGen::visitProgram(const ProgramNode &node) {
	llvm::Function *mainFunc = llvm::Function::Create(
	    llvm::FunctionType::get(builder.getInt32Ty(), false),
	    llvm::Function::ExternalLinkage, "main", *module);
	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", mainFunc);
	builder.SetInsertPoint(entry);
	visitVariableDeclaration(*node.variables);
	visitStatements(*node.statements);
	genPrintVariables();
	for (auto &[name, var_ptr] : variables) {
		auto *var = builder.CreateLoad(builder.getInt8PtrTy(), var_ptr,
		                               "_free_" + name);
		genStrFree(var);
	}
	builder.CreateRet(builder.getInt32(0));

	verify(mainFunc, node.position_begin);
}

LLVMCodeGen::LLVMCodeGen(llvm::LLVMContext &ctx) : ctx(ctx), builder(ctx) {
	module = std::make_unique<llvm::Module>("program", ctx);
}

std::unique_ptr<llvm::Module> LLVMCodeGen::fromAST(llvm::LLVMContext &ctx,
                                                   const ProgramNode &node) {
	LLVMCodeGen codegen(ctx);
	codegen.visitProgram(node);
	return std::move(codegen.module);
}

void LLVMCodeGen::genPrintVariables() {
	auto printfFunc = module->getOrInsertFunction(
	    "printf", llvm::FunctionType::get(builder.getInt32Ty(),
	                                      builder.getInt8PtrTy(), true));
	for (auto &[name, var_ptr] : variables) {
		auto *entry = builder.GetInsertBlock();
		auto *current_func = entry->getParent();
		auto *onnull = llvm::BasicBlock::Create(ctx, "_display_onnull_" + name,
		                                        current_func);
		auto *cont = llvm::BasicBlock::Create(ctx, "_display_cont_" + name,
		                                      current_func);
		auto *var = builder.CreateLoad(builder.getInt8PtrTy(), var_ptr,
		                               "_display_var_" + name);
		auto *isnull = builder.CreateIsNull(var, "_display_isnull_" + name);
		builder.CreateCondBr(isnull, onnull, cont);

		builder.SetInsertPoint(onnull);
		auto *nullalt =
		    builder.CreateGlobalStringPtr("<null>", "_display_nullalt");
		builder.CreateBr(cont);

		builder.SetInsertPoint(cont);
		auto *msg = builder.CreatePHI(builder.getInt8PtrTy(), 2,
		                              "_display_msg_" + name);
		msg->addIncoming(var, entry);
		msg->addIncoming(nullalt, onnull);

		auto *printf_template = builder.CreateGlobalStringPtr(
		    name + " = %s\n", "_display_template_" + name);
		builder.CreateCall(printfFunc, {printf_template, msg});
	}
}

void LLVMCodeGen::verify(llvm::Function *function, int position) {
	std::string err;
	llvm::raw_string_ostream err_stream(err);
	if (llvm::verifyFunction(*function, &err_stream)) {
		throw CompileException(position, err);
	}
}

} // namespace compiler
