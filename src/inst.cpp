/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "inst.h"

namespace optimizer {

ValueRef Literal::toValueRef(Bytecode *byte_code) {
  switch (type()) {
  case LiteralType::ARGUMENT: {
    assert(index() < byte_code->args().argumentEnd());
    return Value::_any();
  }
  case LiteralType::REGISTER: {
    assert(index() < byte_code->args().registerEnd());
    return byte_code->stack().getRegister(index());
  }
  case LiteralType::IDENT: {
    assert(index() < byte_code->args().identEnd());
    return Value::_any();
  }
  case LiteralType::CONSTANT: {
    assert(index() < byte_code->args().constLiteralEnd());
    return byte_code->literalPool().getLiteral(index());
    break;
  }
  case LiteralType::TEMPLATE: {
    return Value::_object();
  }
  default: {
    unreachable();
  }
  }
}

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

Literal Inst::decodeTemplateLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  assert(index >= byteCode()->args().constLiteralEnd() &&
         index < byteCode()->args().literalEnd());

  return {LiteralType::TEMPLATE, index};
}

Literal Inst::decodeStringLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  assert(index >= byteCode()->args().registerEnd() &&
         index < byteCode()->args().identEnd());

  return {LiteralType::IDENT, index};
}

Literal Inst::decodeLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  return decodeLiteral(index);
}

Literal Inst::decodeLiteral(LiteralIndex index) {
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

  stack().resetOperands();

  switch (argument.type()) {
  case OperandType::NONE: {
    break;
  }
  case OperandType::BRANCH: {
    uint32_t offset_length = CBC_BRANCH_OFFSET_LENGTH(byteCode()->current());
    int32_t offset = byteCode()->next();

    if (offset_length > 1) {
      offset <<= 8;
      offset |= byteCode()->next();

      if (offset_length > 2) {
        assert(offset_length == 3);
        offset <<= 8;
        offset |= byteCode()->next();
      }
    }

    if (OpcodeData().isBackwardBrach()) {
      offset = -offset;
    }

    argument.setBranchOffset(offset);
    break;
  }
  case OperandType::STACK: {
    argument.setStackDelta(-1);

    stack().setLeft(stack().pop());
    break;
  }
  case OperandType::STACK_STACK: {
    argument.setStackDelta(-2);

    stack().setRight(stack().pop());
    stack().setLeft(stack().pop());
    break;
  }
  case OperandType::LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);

    stack().setLeft(first_literal.toValueRef(byteCode()));
    break;
  }
  case OperandType::LITERAL_LITERAL: {
    Literal first_literal = decodeLiteral();
    Literal second_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    argument.setSecondLiteral(second_literal);

    stack().setLeft(first_literal.toValueRef(byteCode()));
    stack().setRight(second_literal.toValueRef(byteCode()));
    break;
  }
  case OperandType::STACK_LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);
    argument.setStackDelta(-1);

    stack().setLeft(first_literal.toValueRef(byteCode()));
    stack().setRight(stack().pop());
    break;
  }
  case OperandType::THIS_LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setFirstLiteral(first_literal);

    stack().setLeft(Value::_object());
    stack().setRight(first_literal.toValueRef(byteCode()));
    break;
  }
  default:
    unreachable();
  }

  argument_ = argument;
}

bool Inst::decodeCBCOpcode() {
  CBCOpcode cbc_op = byteCode()->next();
  Opcode opcode(cbc_op);

  if (Opcode::isExtStartOpcode(cbc_op)) {
    if (!byteCode()->hasNext()) {
      return false;
    }

    CBCOpcode cbc_op = byteCode()->next();

    if (Opcode::isEndOpcode(cbc_op)) {
      return false;
    }
    opcode.toExtOpcode(cbc_op);
  }

  cbc_opcode_ = opcode;
  return true;
}
void Inst::processPut() {}

void Inst::decodeGroupOpcode() {
  auto groupOpcode = opcode().opcodeData().groupOpcode();
  switch (groupOpcode) {

  case VM_OC_POP: {
    stack().pop();
    break;
  }
  case VM_OC_POP_BLOCK: {
    stack().setBlockResult(stack().pop());
    break;
  }
  case VM_OC_PUSH: {
    stack().push(stack().left());
    break;
  }
  case VM_OC_PUSH_TWO: {
    stack().push(stack().left());
    stack().push(stack().right());
    break;
  }
  case VM_OC_PUSH_THREE: {
    stack().push(stack().left());
    stack().push(stack().right());

    Literal third_literal = decodeLiteral();
    stack().push(third_literal.toValueRef(byteCode()));
    break;
  }
  case VM_OC_PUSH_UNDEFINED: {
    stack().push(Value::_undefined());
    break;
  }
  case VM_OC_PUSH_TRUE: {
    stack().push(Value::_true());
    break;
  }
  case VM_OC_PUSH_FALSE: {
    stack().push(Value::_false());
    break;
  }
  case VM_OC_PUSH_NULL: {
    stack().push(Value::_null());
    break;
  }
  case VM_OC_PUSH_THIS: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_PUSH_0: {
    stack().push(Value::_number(0));
    break;
  }
  case VM_OC_PUSH_POS_BYTE: {
    stack().push(Value::_number(byteCode()->next() + 1));
    break;
  }
  case VM_OC_PUSH_NEG_BYTE: {
    stack().push(Value::_number(-(byteCode()->next() + 1)));
    break;
  }
  case VM_OC_PUSH_LIT_0: {
    stack().push(stack().left());
    stack().push(Value::_number(0));
    break;
  }
  case VM_OC_PUSH_LIT_POS_BYTE: {
    stack().push(stack().left());
    stack().push(Value::_number(byteCode()->next() + 1));
    break;
  }
  case VM_OC_PUSH_LIT_NEG_BYTE: {
    stack().push(stack().left());
    stack().push(Value::_number(-(byteCode()->next() + 1)));
    break;
  }
  case VM_OC_PUSH_OBJECT: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_PUSH_NAMED_FUNC_EXPR: {
    setLiteralValue(stack().right());
    stack().push(stack().left());
    break;
  }
  case VM_OC_CREATE_BINDING: {
    setStringLiteral();
    break;
  }
  case VM_OC_VAR_EVAL: {
    if (opcode().is(CBC_CREATE_VAR_FUNC_EVAL)) {
      Literal literal = decodeTemplateLiteral();
      setLiteralValue(literal);
    }

    setStringLiteral();
    break;
  }
  case VM_OC_EXT_VAR_EVAL: {
    if (opcode().is(CBC_EXT_CREATE_VAR_FUNC_EVAL)) {
      Literal literal = decodeTemplateLiteral();
      setLiteralValue(literal);
    }

    setStringLiteral();
    break;
  }
  case VM_OC_CREATE_ARGUMENTS: {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {
      stack().setRegister(literal_index, Value::_object());
      break;
    }

    setStringLiteral();
    break;
  }
  case VM_OC_SET_BYTECODE_PTR: {
    break;
  }
  case VM_OC_INIT_ARG_OR_FUNC: {
    LiteralIndex literal_index = decodeLiteralIndex();
    LiteralIndex value_index = decodeLiteralIndex();
    ValueRef lit_value;

    if (value_index < byteCode()->args().registerEnd()) {
      lit_value = stack().getRegister(value_index);
    } else {
      lit_value = Value::_object();
    }

    if (literal_index < byteCode()->args().registerEnd()) {
      stack().setRegister(literal_index, lit_value);
      break;
    }

    setStringLiteral();
    setLiteralValue(lit_value);
    break;
  }
  case VM_OC_CHECK_VAR: {
    setStringLiteral();
    break;
  }
  case VM_OC_CHECK_LET: {
    setStringLiteral();
    break;
  }
  case VM_OC_ASSIGN_LET_CONST: {
    setStringLiteral();
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_INIT_BINDING: {
    setStringLiteral();
    setLiteralValue(stack().pop());
    break;
  }
  case VM_OC_THROW_CONST_ERROR: {
    stack().setResult(Value::_object());
    break;
  }
  case VM_OC_THROW_SYNTAX_ERROR: {
    stack().setResult(Value::_object());
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_COPY_TO_GLOBAL: {
    setStringLiteral();
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_COPY_FROM_ARG: {
    setStringLiteral();
    break;
  }
  case VM_OC_CLONE_CONTEXT: {
    break;
  }
  case VM_OC_SET__PROTO__: {
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_PUSH_STATIC_FIELD_FUNC: {
    ValueRef value = stack().pop();
    stack().shift(4, 3);

    stack().setStack(-4, value);
    if (!opcode().is(CBC_EXT_PUSH_STATIC_COMPUTED_FIELD_FUNC)) {
      break;
    }

    stack().setLeft(value);
    /* FALLTHRU */
  }
  case VM_OC_ADD_COMPUTED_FIELD: {
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_COPY_DATA_PROPERTIES: {
    setLiteralValue(stack().pop());
    break;
  }
  case VM_OC_SET_COMPUTED_PROPERTY: {
    ValueRef tmp = stack().left();
    stack().setRight(stack().right());
    stack().setLeft(tmp);
    /* FALLTHRU */
  }
  case VM_OC_SET_PROPERTY: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    break;
  }
  case VM_OC_PUSH_ARRAY: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_LOCAL_EVAL: {
    setPayload(byteCode()->next());
    break;
  }
  case VM_OC_SUPER_CALL: {
    auto argc = byteCode()->next();
    stack().pop(argc);
    stack().pop(1); // function object
    if (OpcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (OpcodeData().isPutBlock()) {
      stack().setBlockResult(Value::_any());
    }
    break;
  }
  case VM_OC_PUSH_CLASS_ENVIRONMENT: {
    setStringLiteral();
    break;
  }
  case VM_OC_PUSH_IMPLICIT_CTOR: {
    stack().setBlockResult(Value::_object());
    break;
  }
  case VM_OC_INIT_CLASS: {
    ValueRef value = stack().getStack();
    stack().setStack(-2, value);
    stack().setStack(-1, Value::_object());
    break;
  }
  case VM_OC_FINALIZE_CLASS: {
    if (opcode().is(CBC_EXT_FINALIZE_NAMED_CLASS)) {
      stack().setLeft(decodeStringLiteral().toValueRef(byteCode()));
    }

    stack().setStack(-3, stack().getStack(-2));
    stack().pop(2);
    break;
  }
  case VM_OC_SET_FIELD_INIT: {
    break;
  }
  case VM_OC_RUN_FIELD_INIT: {
    break;
  }
  case VM_OC_RUN_STATIC_FIELD_INIT: {
    stack().setLeft(stack().getStack(-2));
    stack().setStack(-2, stack().getStack(-1));
    stack().pop(1);
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_SET_NEXT_COMPUTED_FIELD: {
    stack().setResult(stack().getStack(-2));
    stack().setStack(-1, Value::_any());
    stack().setStack(-2, Value::_any());
    processPut();
    break;
  }
  case VM_OC_PUSH_SUPER_CONSTRUCTOR: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_RESOLVE_LEXICAL_THIS: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_OBJECT_LITERAL_HOME_ENV: {
    if (opcode().is(CBC_EXT_PUSH_OBJECT_SUPER_ENVIRONMENT)) {
      ValueRef obj_value = stack().getStack();
      stack().setStack(-1, Value::_internal());
      stack().push(obj_value);
    } else {
      stack().setStack(-2, stack().getStack(-1));
      stack().pop(1);
    }
    break;
  }
  case VM_OC_SET_HOME_OBJECT: {
    break;
  }
  case VM_OC_SUPER_REFERENCE: {
    // TODO
    break;
  }
  case VM_OC_SET_FUNCTION_NAME: {
    if (!opcode().is(CBC_EXT_SET_FUNCTION_NAME)) {
      if (opcode().is(CBC_EXT_SET_CLASS_NAME)) {
        setStringLiteral();
      } else {
        setStringLiteral(stack().getStack(-2));
      }
    }

    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_PUSH_SPREAD_ELEMENT: {
    stack().push(Value::_internal());
    break;
  }
  case VM_OC_PUSH_REST_OBJECT: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_GET_ITERATOR: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_ITERATOR_STEP: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_ITERATOR_CLOSE: {
    setLiteralValue(stack().left());
    break;
  }
  case VM_OC_DEFAULT_INITIALIZER: {
    // TODO branch
    break;
  }
  case VM_OC_REST_INITIALIZER: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_INITIALIZER_PUSH_LIST: {
    stack().push();
    stack().setStack(-1, stack().getStack(-2));
    stack().setStack(-2, Value::_object());
    break;
  }
  case VM_OC_INITIALIZER_PUSH_REST: {
    setLiteralValue(stack().left());
    stack().setStack(-2, stack().getStack(-1));
    stack().setStack(-1, stack().left());
    break;
  }
  case VM_OC_INITIALIZER_PUSH_NAME: {
    setLiteralValue(stack().left());
    /* FALLTHRU */
  }
  case VM_OC_INITIALIZER_PUSH_PROP: {
    setLiteralValue(stack().left());
    stack().push(Value::_any());
    break;
  }
  case VM_OC_MOVE: {
    auto index = 1 + (opcode().CBCopcode() - CBC_EXT_MOVE);

    ValueRef element = stack().getStack(-index);

    // TODO
    // for (int32_t i = -index; i < -1; i++) {
    //   stack_top_p[i] = stack_top_p[i + 1];
    // }

    stack().setStack(-1, element);
    break;
  }
  case VM_OC_SPREAD_ARGUMENTS: {
    auto argc = byteCode()->next();
    stack().pop(argc);
    stack().pop(opcode().CBCopcode() >= CBC_EXT_SPREAD_CALL_PROP ? 3 : 1);
    stack().push(Value::_internal());

    if (OpcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (OpcodeData().isPutBlock()) {
      stack().setBlockResult(Value::_any());
    }
    break;
  }
  case VM_OC_CREATE_GENERATOR: {
    stack().setResult(Value::_object());
    break;
  }
  case VM_OC_YIELD: {
    stack().setResult(stack().pop());
    break;
  }
  case VM_OC_ASYNC_YIELD: {
    stack().pop(1);
    stack().setResult(Value::_undefined());
    break;
  }
  case VM_OC_ASYNC_YIELD_ITERATOR: {
    setLiteralValue(stack().getStack(-1));
    stack().setBlockResult(Value::_any());
    stack().setResult(Value::_undefined());
    break;
  }
  case VM_OC_AWAIT: {
    stack().setResult(Value::_undefined());
    break;
  }
  case VM_OC_EXT_RETURN: {
    // TODO
    stack().setResult(stack().left());
    break;
  }
  case VM_OC_NONE: {
    break;
  }
  default: {
    break;
  }
  }
}

} // namespace optimizer
