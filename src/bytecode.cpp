/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "bytecode.h"
#include "basic-block.h"
#include "inst.h"
#include "liveness-analyzer.h"

extern "C" {
#include "jerry-snapshot.h"
#include "jerryscript.h"
}

namespace optimizer {

Bytecode::Bytecode(ecma_value_t function) {
  assert(ecma_is_value_object(function));

  auto func = ecma_get_object_from_value(function);
  assert(ecma_get_object_type(func) == ECMA_OBJECT_TYPE_FUNCTION);
  auto ext_func = reinterpret_cast<ecma_extended_object_t *>(func);

  function_ = function;
  compiled_code_ = const_cast<ecma_compiled_code_t *>(
      ecma_op_function_get_compiled_code(ext_func));

  buildInstructions();
}

Bytecode::Bytecode(ecma_compiled_code_t *compiled_code)
    : function_(ECMA_VALUE_UNDEFINED), compiled_code_(compiled_code) {
  buildInstructions();
}

void Bytecode::readSubFunctions(BytecodeList &list, Bytecode *byte_code) {
  if (!byte_code->flags().isFunction()) {
    return;
  }

  for (uint32_t i = byte_code->args().constLiteralEnd();
       i < byte_code->args().literalEnd(); i++) {
    ecma_compiled_code_t *bytecode_literal_p = ECMA_GET_INTERNAL_VALUE_POINTER(
        ecma_compiled_code_t, byte_code->literalPool().literalStart()[i]);

    if (bytecode_literal_p != byte_code->compiledCode()) {
      Bytecode *sub_byte_code = new Bytecode(bytecode_literal_p);
      list.push_back(sub_byte_code);
      readSubFunctions(list, sub_byte_code);
    }
  }
}

size_t Bytecode::countFunctions(std::string snapshot) {
  const uint8_t *snapshot_data = (uint8_t *)snapshot.c_str();
  size_t snapshot_size = snapshot.size();

  if (snapshot_size <= sizeof(jerry_snapshot_header_t)) {
    return SIZE_MAX;
  }

  auto header_p =
      reinterpret_cast<const jerry_snapshot_header_t *>(snapshot_data);

  return header_p->number_of_funcs;
}

BytecodeList Bytecode::readFunctions(ecma_value_t function) {
  BytecodeList list;

  Bytecode *byte_code = new Bytecode(function);
  list.push_back(byte_code);
  readSubFunctions(list, byte_code);

  return list;
}

void Bytecode::setArguments(cbc_uint16_arguments_t *args) {
  args_.setArguments(args);
  byte_code_start_ = literal_pool_.setLiteralPool(args + 1, args_);
  byte_code_ = byte_code_start_;
}

void Bytecode::setArguments(cbc_uint8_arguments_t *args) {
  args_.setArguments(args);
  byte_code_start_ = literal_pool_.setLiteralPool(args + 1, args_);
  byte_code_ = byte_code_start_;
}

void Bytecode::setEncoding() {
  uint16_t limit = CBC_SMALL_LITERAL_ENCODING_LIMIT;
  uint16_t delta = CBC_SMALL_LITERAL_ENCODING_DELTA;

  if (flags_.fullLiteralEncoding()) {
    limit = CBC_FULL_LITERAL_ENCODING_LIMIT,
    delta = CBC_FULL_LITERAL_ENCODING_DELTA;
  }
  args_.setEncoding(limit, delta);
}

void Bytecode::setBytecodeEnd() {
  size_t size = compiledCodesize();

  if (flags().mappedArgumentsNeeded()) {
    size -= args().argumentEnd() * sizeof(ecma_value_t);
  }

  if (flags().hasExtendedInfo()) {
    size -= sizeof(ecma_value_t);
  }

  if (flags().hasName()) {
    size -= sizeof(ecma_value_t);
  }

  if (flags().hasTaggedTemplateLiterals()) {
    size -= sizeof(ecma_value_t);
  }

  byte_code_end_ = reinterpret_cast<uint8_t *>(compiledCode()) + size;
}

void Bytecode::decodeHeader() {
  flags_.setFlags(compiled_code_->status_flags);

  if (flags_.uint16Arguments()) {
    auto args = reinterpret_cast<cbc_uint16_arguments_t *>(compiled_code_);
    setArguments(args);
  } else {
    auto args = reinterpret_cast<cbc_uint8_arguments_t *>(compiled_code_);
    setArguments(args);
  }

  setEncoding();
  setBytecodeEnd();
  stack_ = Stack(args().stackLimit(), args().registerCount());
}

void Bytecode::buildInstructions() {
  decodeHeader();
  LOG("--------- function intructions start --------");

  while (hasNext()) {
    Ins *inst = new Ins(this);
    instructions().push_back(inst);

    if (!inst->decodeCBCOpcode()) {
      delete inst;
      instructions().pop_back();
      break;
    }
    inst->decodeArguments();
    inst->decodeGroupOpcode();
    offsetToInst().insert({inst->offset(), inst});
  }

  auto &inst = instructions().back();

  /* end of bytecode stream */
  if (inst->size() == 0) {
    instructions().back()->setSize(offset() - instructions().back()->offset());
  }

  LOG("--------- function intructions end --------");
}

Bytecode::~Bytecode() {
  for (auto iter : live_ranges_) {
    for (auto li : iter.second) {
      delete li;
    }
  }

  for (auto &bb : bb_list_) {
    delete bb;
  }

  for (auto ins : instructions_) {
    delete ins;
  }

  ecma_free_value(function_);
};

} // namespace optimizer
