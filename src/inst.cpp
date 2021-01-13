/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "inst.h"

namespace optimizer {

#define CBC_OPCODE(arg1, arg2, arg3, arg4) arg4,
uint16_t decode_table[] = {CBC_OPCODE_LIST CBC_EXT_OPCODE_LIST};
#undef CBC_OPCODE

LiteralIndex Inst::decodeLiteralIndex() {
  LiteralIndex literal_index = byteCode()->next();

  if (literal_index >= byteCode()->args().encodingLimit()) {
    literal_index = static_cast<LiteralIndex>(
        (((literal_index) << 8) | byteCode()->next()) -
        byteCode()->args().encodingDelta());
  }

  return literal_index;
}

Literal Inst::decodeLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  LiteralType type;

  if (index < byteCode()->args().argumentEnd()) {
    type = LiteralType::ARGUMENT;
  } else if (index < byteCode()->args().registerEnd()) {
    type = LiteralType::REGISTER;
  } else if (index < byteCode()->args().identEnd()) {
    type = LiteralType::IDENT;
  } else if (index < byteCode()->args().constLiteralEnd()) {
    type = LiteralType::CONSTANT;
  } else {
    assert(index < byteCode()->args().literalEnd());
    type = LiteralType::TEMPLATE;
  }

  return {type, index};
}

void Inst::decodeArguments() {
  Argument argument(opcode().opcodeData().operands());

  switch (argument.type()) {
  case OperandType::NONE: {
    break;
  }
  case OperandType::BRANCH: {
    uint32_t offset_length = CBC_BRANCH_OFFSET_LENGTH(byteCode()->current());
    uint32_t offset = byteCode()->next();

    if (offset_length > 1) {
      offset <<= 8;
      offset |= byteCode()->next();

      if (offset_length > 2) {
        assert(offset_length == 3);
        offset <<= 8;
        offset |= byteCode()->next();
      }
    }

    argument.setBranchOffset(offset);
    break;
  }
  case OperandType::STACK: {
    argument.setStackDelta(-1);
    break;
  }
  case OperandType::STACK_STACK: {
    argument.setStackDelta(-2);
    break;
  }
  case OperandType::LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    break;
  }
  case OperandType::LITERAL_LITERAL: {
    Literal first_literal = decodeLiteral();
    Literal second_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    argument.setSecondLiteral(second_literal);
    break;
  }
  case OperandType::STACK_LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    argument.setStackDelta(-1);
    break;
  }
  case OperandType::THIS_LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    break;
  }
  default:
    unreachable();
  }

  argument_ = argument;
}

void Inst::decodeCBCOpcode() {
  CBCOpcode cbc_op = byteCode()->next();
  Opcode opcode(cbc_op);

  if (cbc_op == CBC_EXT_OPCODE) {
    CBCOpcode cbc_op = byteCode()->next();
    opcode.toExtOpcode(cbc_op);
  }

  opcode_ = opcode;
}

void Inst::decodeGroupOpcode() {
  auto groupOpcode = opcode().opcodeData().groupOpcode();
  switch (groupOpcode) {
  case VM_OC_VAR_EVAL: {

    break;
  }
  default: {

    break;
  }
  }
}

} // namespace optimizer
