/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_CORE_FRAMEWORK_OP_EXPR_H_
#define ONEFLOW_CORE_FRAMEWORK_OP_EXPR_H_

#include "oneflow/core/framework/user_op_conf.pb.h"

namespace oneflow {
namespace one {

class Tensor {};
using TensorRef = std::shared_ptr<Tensor>;
using TensorList = std::vector<TensorRef>;

#define DEFINE_DEFAULT_CONSTRUCTOR(class_type) \
  class_type() = default;                      \
  virtual ~class_type() = default;

class OpExpr {
 public:
  DEFINE_DEFAULT_CONSTRUCTOR(OpExpr);

  // TODO(): Uncomment.
  // virtual FilterInputTensorsUsedByBackward(const TensorList& inputs) = 0;
  // virtual FilterOutputTensorsUsedByBackward(const TensorList& outputs) = 0;

  virtual std::shared_ptr<OpExpr> GetBackwardOpExpr() const = 0;

  virtual std::string type() const = 0;
};

class BuiltinOpExpr : public OpExpr {
 public:
  DEFINE_DEFAULT_CONSTRUCTOR(BuiltinOpExpr);

  explicit BuiltinOpExpr(const std::string& op_name) : op_name_(op_name) {}

  const std::string& op_name() const { return op_name_; }
  void set_op_name(const std::string& op_name) { op_name_ = op_name; }

 private:
  std::string op_name_;
};

class UserOpExpr : public BuiltinOpExpr {
 public:
  DEFINE_DEFAULT_CONSTRUCTOR(UserOpExpr);

  explicit UserOpExpr(const std::string& op_name) : BuiltinOpExpr(op_name) {}

  std::shared_ptr<OpExpr> GetBackwardOpExpr() const override;

  std::string type() const override { return "UserOp"; }

  const UserOpConf& proto() const { return proto_; }
  UserOpConf* mutable_proto() { return &proto_; }

  const std::vector<std::string>& indexed_input_names() const { return indexed_input_names_; }

  friend class OpBuilder;

 private:
  // The internal operation proto.
  UserOpConf proto_;

  // The indexed input operand names.
  std::vector<std::string> indexed_input_names_;
};

// TODO(): Finish the class definition of `FunctionOpExpr`.
class FunctionOpExpr : public OpExpr {
 public:
  DEFINE_DEFAULT_CONSTRUCTOR(FunctionOpExpr);

  std::shared_ptr<OpExpr> GetBackwardOpExpr() const override;

  std::string type() const override { return "FunctionOp"; }
};

}  // namespace one
}  // namespace oneflow

#endif  // ONEFLOW_CORE_FRAMEWORK_OP_EXPR_H_