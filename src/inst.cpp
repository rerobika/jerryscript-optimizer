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
    uint32_t reg_index = byte_code->toRegisterIndex(index());
    byte_code->instructions().back()->addReadReg(reg_index);
    return byte_code->stack().getRegister(reg_index);
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

#define CBC_OPCODE(arg1, arg2, arg3, arg4) #arg1,
const char *cbc_names[] = {CBC_OPCODE_LIST CBC_EXT_OPCODE_LIST};
#undef CBC_OPCODE

LiteralIndex Ins::decodeLiteralIndex() {
  LiteralIndex literal_index = byteCode()->next();

  if (literal_index >= byteCode()->args().encodingLimit()) {
    literal_index = static_cast<LiteralIndex>(
        (((literal_index) << 8) | byteCode()->next()) -
        byteCode()->args().encodingDelta());
  }

  return literal_index;
}

Literal Ins::decodeTemplateLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  assert(index >= byteCode()->args().constLiteralEnd() &&
         index < byteCode()->args().literalEnd());

  return {LiteralType::TEMPLATE, index};
}

Literal Ins::decodeStringLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  assert(index >= byteCode()->args().registerEnd() &&
         index < byteCode()->args().identEnd());

  return {LiteralType::IDENT, index};
}

Literal Ins::decodeLiteral() {
  LiteralIndex index = decodeLiteralIndex();
  return decodeLiteral(index);
}

Literal Ins::decodeLiteral(LiteralIndex index) {
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

  Literal lit{type, index};

  if (index >= byteCode()->args().registerEnd()) {
    argument_.addLiteral(lit);
  }

  return lit;
}

void Ins::decodeArguments() {
  Argument argument(opcode().opcodeData().operands());
  argument_ = argument;

  stack().resetOperands();

  switch (argument.type()) {
  case OperandType::NONE: {
    break;
  }
  case OperandType::BRANCH: {
    uint32_t offset_length = CBC_BRANCH_OFFSET_LENGTH(opcode().CBCopcode());
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

    if (opcode().opcodeData().isBackwardBrach()) {
      offset = -offset;
    }

    argument_.setBranchOffset(offset);
    break;
  }
  case OperandType::STACK: {
    argument_.setStackDelta(-1);

    stack().setLeft(stack().pop());
    break;
  }
  case OperandType::STACK_STACK: {
    argument_.setStackDelta(-2);

    stack().setRight(stack().pop());
    stack().setLeft(stack().pop());
    break;
  }
  case OperandType::LITERAL: {
    Literal first_literal = decodeLiteral();

    stack().setLeft(first_literal.toValueRef(byteCode()));
    break;
  }
  case OperandType::LITERAL_LITERAL: {
    Literal first_literal = decodeLiteral();
    Literal second_literal = decodeLiteral();

    stack().setLeft(first_literal.toValueRef(byteCode()));
    stack().setRight(second_literal.toValueRef(byteCode()));
    break;
  }
  case OperandType::STACK_LITERAL: {
    Literal first_literal = decodeLiteral();
    argument.setStackDelta(-1);

    stack().setLeft(first_literal.toValueRef(byteCode()));
    stack().setRight(stack().pop());
    break;
  }
  case OperandType::THIS_LITERAL: {
    Literal first_literal = decodeLiteral();

    stack().setLeft(Value::_object());
    stack().setRight(first_literal.toValueRef(byteCode()));
    break;
  }
  default:
    unreachable();
  }
}

bool Ins::decodeCBCOpcode() {
  setOffset(byteCode()->offset());
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

  opcode_ = opcode;

  LOG(*this);

  return true;
}

void Ins::processPut() {
  if (opcode().opcodeData().isPutIdent()) {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {
      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      setWriteReg(reg_index);
      stack().setRegister(reg_index, stack().result());
    } else {
      Literal literal = decodeLiteral(literal_index);
      setStringLiteral(literal);
      setLiteralValue(stack().result());
    }
  } else if (opcode().opcodeData().isPutReference()) {
    ValueRef property = stack().pop();
    ValueRef base = stack().pop();

    if (base->type() == ValueType::INTERNAL) {
      uint32_t literal_index = ecma_get_integer_from_value(property->value());
      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      setWriteReg(reg_index);
      stack().setRegister(reg_index, stack().result());
    } else {
      stack().setResult(Value::_any());
    }

    if (!opcode().opcodeData().isPutStack() &&
        !opcode().opcodeData().isPutBlock()) {
      return;
    }
  }

  if (opcode().opcodeData().isPutStack()) {
    stack().push(Value::_any());
  } else if (opcode().opcodeData().isPutBlock()) {
    stack().setBlockResult(Value::_any());
  }
}

void Ins::decodeGroupOpcode() {
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
#if ENABLED(JERRY_ESNEXT)
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
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_CREATE_ARGUMENTS: {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {
      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      setWriteReg(reg_index);
      stack().setRegister(reg_index, Value::_object());
      break;
    }

    setStringLiteral();
    break;
  }
#if ENABLED(JERRY_SNAPSHOT_EXEC)
  case VM_OC_SET_BYTECODE_PTR: {
    break;
  }
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */
  case VM_OC_INIT_ARG_OR_FUNC: {
    LiteralIndex literal_index = decodeLiteralIndex();
    LiteralIndex value_index = decodeLiteralIndex();
    ValueRef lit_value;

    if (value_index < byteCode()->args().registerEnd()) {
      uint32_t reg_index = byteCode()->toRegisterIndex(value_index);
      addReadReg(reg_index);
      lit_value = stack().getRegister(reg_index);
    } else {
      lit_value = Value::_object();
    }

    if (literal_index < byteCode()->args().registerEnd()) {
      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      setWriteReg(reg_index);
      stack().setRegister(reg_index, lit_value);
      break;
    }

    setStringLiteral();
    setLiteralValue(lit_value);
    break;
  }
#if ENABLED(JERRY_ESNEXT)
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
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_SET_PROPERTY: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    break;
  }
  case VM_OC_PUSH_ARRAY: {
    stack().push(Value::_object());
    break;
  }
#if ENABLED(JERRY_ESNEXT)
  case VM_OC_LOCAL_EVAL: {
    setPayload(byteCode()->next());
    break;
  }
  case VM_OC_SUPER_CALL: {
    auto argc = byteCode()->next();
    stack().pop(argc);
    stack().pop(1); // function object
    if (opcode().opcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (opcode().opcodeData().isPutBlock()) {
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

    if (opcode().opcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (opcode().opcodeData().isPutBlock()) {
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
  case VM_OC_GENERATOR_AWAIT: {
    stack().setResult(Value::_undefined());
    break;
  }
  case VM_OC_EXT_RETURN: {
    // TODO: full support
    stack().setResult(stack().left());
    break;
  }
  case VM_OC_ASYNC_EXIT: {
    // TODO: full support
    stack().setBlockResult(Value::_undefined());
    stack().setResult(Value::_object());
    break;
  }
  case VM_OC_STRING_CONCAT: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_string());
    break;
  }
  case VM_OC_GET_TEMPLATE_OBJECT: {
    stack().push(Value::_object());
    break;
  }
  case VM_OC_PUSH_NEW_TARGET: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_REQUIRE_OBJECT_COERCIBLE: {
    setLiteralValue(stack().getStack(-1));
    break;
  }
  case VM_OC_ASSIGN_SUPER: {
    // TODO: full support
    break;
  }
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_PUSH_ELISON: {
    stack().push(Value::_internal());
    break;
  }
  case VM_OC_APPEND_ARRAY: {
    auto values_length = byteCode()->next();
    stack().pop(values_length);
    break;
  }
  case VM_OC_IDENT_REFERENCE: {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {
      stack().push(Value::_internal());
      stack().push(Value::_number(literal_index));

      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      addReadReg(reg_index);
      stack().push(stack().getRegister(reg_index));
    } else {
      stack().push(Value::_object());
      stack().push(Value::_string());
      stack().push(Value::_any());
    }
    break;
  }
  case VM_OC_PROP_GET: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_PROP_REFERENCE: {
    if (opcode().is(CBC_PUSH_PROP_REFERENCE)) {
      stack().setLeft(stack().getStack(-2));
      stack().setRight(stack().getStack(-1));
    } else if (opcode().is(CBC_PUSH_PROP_LITERAL_REFERENCE)) {
      stack().push(stack().left());
      stack().setRight(stack().left());
      stack().setLeft(stack().getStack(-2));
    } else {
      stack().push(stack().left());
      stack().push(stack().right());
    }
    /* FALLTHRU */
  }
  case VM_OC_PROP_PRE_INCR:
  case VM_OC_PROP_PRE_DECR:
  case VM_OC_PROP_POST_INCR:
  case VM_OC_PROP_POST_DECR: {
    stack().setResult(Value::_any());
    if (opcode().CBCopcode() < CBC_PRE_INCR) {
      stack().setRight(Value::_undefined());
      processPut();
      break;
    }
    stack().push(2);
    stack().setLeft(stack().result());
    stack().setRight(Value::_undefined());
    /* FALLTHRU */
  }
  case VM_OC_PRE_INCR:
  case VM_OC_PRE_DECR:
  case VM_OC_POST_INCR:
  case VM_OC_POST_DECR: {
    uint32_t opcode_flags = groupOpcode - VM_OC_PROP_PRE_INCR;

    byteCode()->byteCodeCurrent() = byteCode()->byteCodeStart() + offset() + 1;

    if (opcode_flags & VM_OC_POST_INCR_DECR_OPERATOR_FLAG) {
      if (opcode().opcodeData().isPutStack()) {
        if (opcode_flags & VM_OC_IDENT_INCR_DECR_OPERATOR_FLAG) {
          stack().push(stack().result());
        } else {
          stack().push();
          stack().setStack(-1, stack().getStack(-2));
          stack().setStack(-2, stack().getStack(-3));
          stack().setStack(-3, stack().result());
        }
        opcode().opcodeData().removeFlag(ResultFlag::STACK);
      } else {
        stack().setBlockResult(stack().result());
        opcode().opcodeData().removeFlag(ResultFlag::BLOCK);
      }
    }
    stack().setResult(Value::_any());
    processPut();
    break;
  }
  case VM_OC_ASSIGN: {
    stack().setResult(stack().left());
    stack().setLeft(Value::_undefined());
    processPut();
    break;
  }
  case VM_OC_MOV_IDENT: {
    LiteralIndex literal_index = decodeLiteralIndex();
    uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
    setWriteReg(reg_index);
    stack().setRegister(reg_index, stack().left());
    break;
  }
  case VM_OC_ASSIGN_PROP: {
    stack().setResult(stack().getStack(-1));
    stack().setStack(-1, stack().left());
    stack().setLeft(Value::_undefined());
    processPut();
    break;
  }
  case VM_OC_ASSIGN_PROP_THIS: {
    stack().setResult(stack().getStack(-1));
    stack().setStack(-1, Value::_any());
    stack().push(stack().left());
    stack().setLeft(Value::_undefined());
    processPut();
    break;
  }
  case VM_OC_RETURN: {
    if (opcode().is(CBC_RETURN_WITH_BLOCK)) {
      stack().setLeft(stack().blockResult());
      stack().setBlockResult(Value::_undefined());
    }

    stack().setResult(stack().left());
    stack().setLeft(Value::_undefined());
    break;
  }
  case VM_OC_THROW: {
    setLiteralValue(stack().left());
    stack().setResult(Value::_internal());
    stack().setLeft(Value::_undefined());
    break;
  }
  case VM_OC_THROW_REFERENCE_ERROR: {
    stack().setResult(Value::_internal());
    break;
  }
  case VM_OC_EVAL: {
    break;
  }
  case VM_OC_CALL: {
    uint32_t argc;
    if (opcode().CBCopcode() >= CBC_CALL0) {
      argc = (uint32_t)((opcode().CBCopcode() - CBC_CALL0) / 6);
    } else {
      argc = byteCode()->next();
    }

    if (((opcode().CBCopcode() - CBC_CALL) % 6) >= 3) {
      stack().pop(2); // this, prop_name
    }

    stack().pop(argc);
    stack().pop(1); // function object
    if (opcode().opcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (opcode().opcodeData().isPutBlock()) {
      stack().setBlockResult(Value::_any());
    }
    break;
  }
  case VM_OC_NEW: {
    uint32_t argc;
    if (opcode().CBCopcode() >= CBC_NEW0) {
      argc = (uint32_t)((opcode().CBCopcode() - CBC_NEW0));
    } else {
      argc = byteCode()->next();
    }

    stack().pop(argc);
    stack().pop(1); // function object
    if (opcode().opcodeData().isPutStack()) {
      stack().push(Value::_any());
    } else if (opcode().opcodeData().isPutBlock()) {
      stack().setBlockResult(Value::_any());
    }
    break;
  }
  case VM_OC_ERROR: {
    stack().setResult(Value::_internal());
    break;
  }
  case VM_OC_RESOLVE_BASE_FOR_CALL: {
    break;
  }
  case VM_OC_PROP_DELETE: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_boolean());
    break;
  }
  case VM_OC_DELETE: {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {
      stack().push(Value::_false());
    } else {
      stack().push(Value::_boolean());
    }
    break;
  }
  case VM_OC_JUMP: {
    addFlag(InstFlags::JUMP);
    break;
  }
  case VM_OC_BRANCH_IF_STRICT_EQUAL: {
    setLiteralValue(stack().pop());
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    break;
  }
  case VM_OC_BRANCH_IF_TRUE:
  case VM_OC_BRANCH_IF_FALSE:
  case VM_OC_BRANCH_IF_LOGICAL_TRUE:
  case VM_OC_BRANCH_IF_LOGICAL_FALSE: {
    setLiteralValue(stack().pop());
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    break;
  }
#if ENABLED(JERRY_ESNEXT)
  case VM_OC_BRANCH_IF_NULLISH: {
    setLiteralValue(stack().getStack(-1));
    addFlag(InstFlags::CONDITIONAL_JUMP);
    break;
  }
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_PLUS:
  case VM_OC_MINUS: {
    stack().push(Value::_number());
    break;
  }
  case VM_OC_NOT: {
    stack().push(Value::_boolean());
    break;
  }
  case VM_OC_BIT_NOT: {
    stack().push(Value::_number());
    break;
  }
  case VM_OC_VOID: {
    stack().push(Value::_undefined());
    break;
  }
  case VM_OC_TYPEOF_IDENT: {
    LiteralIndex literal_index = decodeLiteralIndex();

    if (literal_index < byteCode()->args().registerEnd()) {

      uint32_t reg_index = byteCode()->toRegisterIndex(literal_index);
      addReadReg(reg_index);
      stack().setLeft(stack().getRegister(reg_index));
    } else {
      stack().setLeft(Value::_any());
    }
    /* FALLTHRU */
  }
  case VM_OC_TYPEOF: {
    setLiteralValue(stack().left());
    stack().push(Value::_string());
    break;
  }
  case VM_OC_ADD: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_any());
    break;
  }
  case VM_OC_SUB:
  case VM_OC_MUL:
  case VM_OC_DIV:
#if ENABLED(JERRY_ESNEXT)
  case VM_OC_EXP:
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_MOD: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_number());
    break;
  }
  case VM_OC_EQUAL:
  case VM_OC_NOT_EQUAL:
  case VM_OC_STRICT_EQUAL:
  case VM_OC_STRICT_NOT_EQUAL: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_boolean());
    break;
  }
  case VM_OC_BIT_OR:
  case VM_OC_BIT_XOR:
  case VM_OC_BIT_AND:
  case VM_OC_LEFT_SHIFT:
  case VM_OC_RIGHT_SHIFT:
  case VM_OC_UNS_RIGHT_SHIFT: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_number());
    break;
  }
  case VM_OC_LESS:
  case VM_OC_GREATER:
  case VM_OC_LESS_EQUAL:
  case VM_OC_GREATER_EQUAL:
  case VM_OC_IN:
  case VM_OC_INSTANCEOF: {
    setStringLiteral(stack().left());
    setLiteralValue(stack().right());
    stack().push(Value::_boolean());
    break;
  }
  case VM_OC_BLOCK_CREATE_CONTEXT: {
#if ENABLED(JERRY_ESNEXT)
    stack().push(PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
#endif /* ENABLED (JERRY_ESNEXT) */
    break;
  }
  case VM_OC_WITH: {
    setLiteralValue(stack().pop());
    stack().push(PARSER_BLOCK_CONTEXT_STACK_ALLOCATION);
    break;
  }
  case VM_OC_FOR_IN_INIT: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    setLiteralValue(stack().pop());
    stack().push(PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION);
    break;
  }
  case VM_OC_FOR_IN_GET_NEXT: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_FOR_IN_HAS_NEXT: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    break;
  }
#if ENABLED(JERRY_ESNEXT)
  case VM_OC_FOR_OF_INIT: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    setLiteralValue(stack().pop());
    stack().push(PARSER_FOR_OF_CONTEXT_STACK_ALLOCATION);
    break;
  }
  case VM_OC_FOR_OF_GET_NEXT: {
    stack().push(Value::_any());
    break;
  }
  case VM_OC_FOR_OF_HAS_NEXT: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    break;
  }
  case VM_OC_FOR_AWAIT_OF_INIT: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::CONDITIONAL_JUMP);
    setLiteralValue(stack().pop());
    stack().push(PARSER_FOR_AWAIT_OF_CONTEXT_STACK_ALLOCATION);
    break;
  }
  case VM_OC_FOR_AWAIT_OF_HAS_NEXT: {
    stack().push(PARSER_TRY_CONTEXT_STACK_ALLOCATION);
    break;
  }
#endif /* ENABLED (JERRY_ESNEXT) */
  case VM_OC_TRY: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::TRY_START);
    break;
  }
  case VM_OC_CATCH: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::TRY_CATCH);
    break;
  }
  case VM_OC_FINALLY: {
    addFlag(InstFlags::JUMP);
    addFlag(InstFlags::TRY_FINALLY);
    stack().push(PARSER_FINALLY_CONTEXT_EXTRA_STACK_ALLOCATION);
    break;
  }
  case VM_OC_CONTEXT_END: {
    // TODO: support
    break;
  }
  case VM_OC_JUMP_AND_EXIT_CONTEXT: {
    // TODO: support
    addFlag(InstFlags::JUMP);
    break;
  }
#if ENABLED(JERRY_DEBUGGER)
  case VM_OC_BREAKPOINT_ENABLED: {
    break;
  }
  case VM_OC_BREAKPOINT_DISABLED: {
    break;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */
#if ENABLED(JERRY_LINE_INFO)
  case VM_OC_LINE: {
    uint32_t value = 0;
    uint8_t byte;

    do {
      byte = byteCode()->next();
      value = (value << 7) | (byte & CBC_LOWER_SEVEN_BIT_MASK);
    } while (byte & CBC_HIGHEST_BIT_MASK);

    break;
  }
#endif /* ENABLED (JERRY_LINE_INFO) */
  case VM_OC_NONE:
  default: {
    break;
  }
  }
} // namespace optimizer

} // namespace optimizer
