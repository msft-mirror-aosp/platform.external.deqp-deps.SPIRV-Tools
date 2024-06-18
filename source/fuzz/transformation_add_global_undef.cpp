// Copyright (c) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "source/fuzz/transformation_add_global_undef.h"

#include "source/fuzz/fuzzer_util.h"
#include "source/opt/reflect.h"

namespace spvtools {
namespace fuzz {

TransformationAddGlobalUndef::TransformationAddGlobalUndef(
    spvtools::fuzz::protobufs::TransformationAddGlobalUndef message)
    : message_(std::move(message)) {}

TransformationAddGlobalUndef::TransformationAddGlobalUndef(uint32_t fresh_id,
                                                           uint32_t type_id) {
  message_.set_fresh_id(fresh_id);
  message_.set_type_id(type_id);
}

bool TransformationAddGlobalUndef::IsApplicable(
    opt::IRContext* ir_context, const TransformationContext& /*unused*/) const {
  // A fresh id is required.
  if (!fuzzerutil::IsFreshId(ir_context, message_.fresh_id())) {
    return false;
  }
  auto type = ir_context->get_def_use_mgr()->GetDef(message_.type_id());
  // The type must exist, and must not be a function or pointer type.
  return type != nullptr && opt::IsTypeInst(type->opcode()) &&
         type->opcode() != spv::Op::OpTypeFunction &&
         type->opcode() != spv::Op::OpTypePointer;
}

void TransformationAddGlobalUndef::Apply(
    opt::IRContext* ir_context, TransformationContext* /*unused*/) const {
  auto new_instruction = MakeUnique<opt::Instruction>(
      ir_context, spv::Op::OpUndef, message_.type_id(), message_.fresh_id(),
      opt::Instruction::OperandList());
  auto new_instruction_ptr = new_instruction.get();
  ir_context->module()->AddGlobalValue(std::move(new_instruction));
  fuzzerutil::UpdateModuleIdBound(ir_context, message_.fresh_id());
  // Inform the def-use manager about the new instruction.
  ir_context->get_def_use_mgr()->AnalyzeInstDef(new_instruction_ptr);
}

protobufs::Transformation TransformationAddGlobalUndef::ToMessage() const {
  protobufs::Transformation result;
  *result.mutable_add_global_undef() = message_;
  return result;
}

std::unordered_set<uint32_t> TransformationAddGlobalUndef::GetFreshIds() const {
  return {message_.fresh_id()};
}

}  // namespace fuzz
}  // namespace spvtools
