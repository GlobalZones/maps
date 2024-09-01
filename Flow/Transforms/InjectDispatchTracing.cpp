// Copyright 2020 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <utility>

#include "iree/compiler/Dialect/Flow/IR/FlowOps.h"
#include "iree/compiler/Dialect/Flow/Transforms/Passes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Pass/Pass.h"

namespace mlir::iree_compiler::IREE::Flow {

#define GEN_PASS_DEF_INJECTDISPATCHTRACINGPASS
#include "iree/compiler/Dialect/Flow/Transforms/Passes.h.inc"

static SmallVector<Value> filterTensorValues(ValueRange &&range) {
  SmallVector<Value> result;
  for (auto value : range) {
    if (llvm::isa<TensorType>(value.getType()))
      result.push_back(value);
  }
  return result;
}

namespace {

struct InjectDispatchTracingPass
    : public IREE::Flow::impl::InjectDispatchTracingPassBase<
          InjectDispatchTracingPass> {
  void runOnOperation() override {
    auto funcOp = getOperation();
    for (auto dispatchOp : funcOp.getFunctionBody().getOps<DispatchOp>()) {
      std::string entryPointName = dispatchOp.getEntryPointName();

      // Input tensors:
      OpBuilder builder(dispatchOp);
      builder.create<IREE::Flow::TensorTraceOp>(
          dispatchOp.getLoc(),
          builder.getStringAttr(entryPointName + " inputs"),
          filterTensorValues(dispatchOp.getArguments()));

      // Output tensors:
      builder.setInsertionPointAfter(dispatchOp);
      builder.create<IREE::Flow::TensorTraceOp>(
          dispatchOp.getLoc(),
          builder.getStringAttr(entryPointName + " outputs"),
          filterTensorValues(dispatchOp.getResults()));
    }
  }
};

} // namespace

} // namespace mlir::iree_compiler::IREE::Flow
