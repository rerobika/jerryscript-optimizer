/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "bytecode.h"
#include "inst.h"

namespace optimizer {

Bytecode::Bytecode(ecma_value_t function) {
  assert(ecma_is_value_object(function));

  function_ = ecma_get_object_from_value(function);
  assert(ecma_get_object_type(function_) == ECMA_OBJECT_TYPE_FUNCTION);
  auto ext_func = reinterpret_cast<ecma_extended_object_t *>(function_);
  compiled_code_ = const_cast<ecma_compiled_code_t *>(
      ecma_op_function_get_compiled_code(ext_func));

  decodeHeader();
  buildInstructions();
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
  byte_code_end_ = reinterpret_cast<uint8_t *>(
      ecma_compiled_code_resolve_arguments_start(compiled_code_));
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
}

void Bytecode::buildInstructions() {
  while (byte_code_ < byte_code_end_) {
    Inst inst(this);

    if (!inst.decodeCBCOpcode()) {
      break;
    }
    inst.decodeArguments();
    inst.decodeGroupOpcode();
  }
}

Bytecode::~Bytecode() { ecma_free_object(ecma_make_object_value(function_)); };

} // namespace optimizer
