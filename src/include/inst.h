/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef INST_H
#define INST_H

#ifndef DUMP_INST
#define DUMP_INST 1
#endif

extern "C" {
#include "jerryscript-config.h"
#include "vm.h"
}
#include "bytecode.h"

namespace optimizer {

extern uint16_t decode_table[];
extern const char *cbc_names[];

class Bytecode;

enum class LiteralType {
  ARGUMENT,
  REGISTER,
  IDENT,
  CONSTANT,
  TEMPLATE,
  LITERAL_TYPE__COUNT,
};

enum class OperandType {
  NONE,
  BRANCH,
  STACK,
  STACK_STACK,
  LITERAL,
  LITERAL_LITERAL,
  STACK_LITERAL,
  THIS_LITERAL,
  OPERAND_TYPE__COUNT,
};

enum class OperandFlags {
  BACKWARD_BRANCH = VM_OC_BACKWARD_BRANCH,
};

enum class ResultFlag {
  IDENT = (1 << 0),
  REFERENCE = (1 << 1),
  STACK = (1 << 2),
  BLOCK = (1 << 3),
  RESULT_TYPE__COUNT
};

using CBCOpcode = uint16_t;
using GroupOpcode = vm_oc_types;

class Literal {
public:
  Literal() : Literal(LiteralType::LITERAL_TYPE__COUNT, 0) {}
  Literal(LiteralType type, LiteralIndex index) : type_(type), index_(index) {}

  ValueRef toValueRef(Bytecode *byte_code);

  auto type() const { return type_; }
  auto index() const { return index_; }

private:
  LiteralType type_;
  LiteralIndex index_;
};

class Argument {
public:
  Argument() : Argument(OperandType::OPERAND_TYPE__COUNT) {}
  Argument(OperandType type) : type_(type), branch_offset_(0) {}

  auto branchOffset() const { return branch_offset_; }
  auto type() const { return type_; }
  auto stackDelta() const { return stack_delta_; }
  auto &firstLiteral() { return first_literal_; }
  auto &secondLiteral() { return second_literal_; }

  void setBranchOffset(int32_t offset) {
    assert(type() == OperandType::BRANCH);
    branch_offset_ = offset;
  }

  void setStackDelta(int32_t delta) {
    assert(type() == OperandType::STACK || type() == OperandType::STACK_STACK ||
           type() == OperandType::STACK_LITERAL);
    stack_delta_ = delta;
  }

  void setFirstLiteral(Literal &literal) {
    assert(isLiteral());
    first_literal_ = literal;
  }

  void setSecondLiteral(Literal &literal) {
    assert(type() == OperandType::LITERAL_LITERAL);
    second_literal_ = literal;
  }

  bool isLiteral() const { return type() >= OperandType::LITERAL; }

private:
  OperandType type_;
  int32_t branch_offset_;
  int32_t stack_delta_;
  Literal first_literal_;
  Literal second_literal_;
};

class OpcodeData {
public:
  OpcodeData() : OpcodeData(0) {}
  OpcodeData(uint32_t data) : data_(data) {}

  auto data() const { return data_; }

  OperandType operands() const {
    return static_cast<OperandType>((data() >> VM_OC_GET_ARGS_SHIFT) &
                                    VM_OC_GET_ARGS_MASK);
  }

  GroupOpcode groupOpcode() const {
    return static_cast<GroupOpcode>((data() & VM_OC_GROUP_MASK));
  }

  ResultFlag result() const { return static_cast<ResultFlag>(putResult()); }

  bool isPutStack() {
    return (putResult() & static_cast<uint32_t>(ResultFlag::STACK)) != 0;
  }

  bool isPutBlock() {
    return (putResult() & static_cast<uint32_t>(ResultFlag::BLOCK)) != 0;
  }

  bool isPutIdent() {
    return (putResult() & static_cast<uint32_t>(ResultFlag::IDENT)) != 0;
  }

  bool isPutReference() {
    return (putResult() & static_cast<uint32_t>(ResultFlag::REFERENCE)) != 0;
  }

  bool isBackwardBrach() const {
    return (data() & static_cast<uint32_t>(OperandFlags::BACKWARD_BRANCH)) != 0;
  }

  bool isForwardBrach() const { return !isBackwardBrach(); }

  void removeFlag(ResultFlag flag) {
    data_ &= ~static_cast<uint32_t>(ResultFlag::STACK);
  }

private:
  uint32_t putResult() const {
    return (data() >> VM_OC_PUT_RESULT_SHIFT) & VM_OC_PUT_RESULT_MASK;
  }

  uint32_t data_;
};

class Opcode {
public:
  Opcode() : Opcode(0) {}
  Opcode(CBCOpcode opcode)
      : cbc_opcode_(opcode), opcode_data_(decode_table[opcode]) {}

  auto CBCopcode() const { return cbc_opcode_; }
  auto opcodeData() const { return opcode_data_; }

  bool is(CBCOpcode opcode) { return cbc_opcode_ == opcode; }

  bool isExtOpcode() const { return Opcode::isExtOpcode(CBCopcode()); }

  static bool isExtOpcode(CBCOpcode opcode) { return opcode > CBC_END; }
  static bool isEndOpcode(CBCOpcode opcode) { return opcode == CBC_EXT_NOP; }
  static bool isExtStartOpcode(CBCOpcode opcode) {
    return opcode == CBC_EXT_OPCODE;
  }

  void toExtOpcode(CBCOpcode cbc_op) {
    assert(Opcode::isExtStartOpcode(CBCopcode()));
    assert(!Opcode::isEndOpcode(cbc_op));
    opcode_data_ = decode_table[cbc_op + CBC_END + 1];
    cbc_opcode_ = cbc_op + 256;
  }

private:
  CBCOpcode cbc_opcode_;
  OpcodeData opcode_data_;
};

class Inst {
public:
  Inst(Bytecode *byte_code)
      : byte_code_(byte_code), stack_snapshot_(nullptr),
        string_literal_(Value::_undefined()),
        literal_value_(Value::_undefined()) {}

  ~Inst() { delete stack_snapshot_; }

  auto byteCode() const { return byte_code_; }
  auto opcode() const { return cbc_opcode_; }
  auto argument() const { return argument_; }
  auto stackSnapshot() const { return stack_snapshot_; }
  auto &stack() { return byteCode()->stack(); }

  void setStringLiteral() {
    Literal literal = decodeStringLiteral();
    setStringLiteral(literal);
  }

  void setStringLiteral(Literal &string_literal) {
    string_literal_ = string_literal.toValueRef(byteCode());
  }

  void setStringLiteral(ValueRef literal_value) {
    string_literal_ = literal_value;
  }

  void setLiteralValue(ValueRef literal_value) {
    literal_value_ = literal_value;
  }

  void setLiteralValue(Literal &literal_value) {
    setLiteralValue(literal_value.toValueRef(byteCode()));
  }

  void setPayload(uint32_t payload) { payload_ = payload; }

  LiteralIndex decodeLiteralIndex();
  Literal decodeTemplateLiteral();
  Literal decodeStringLiteral();
  Literal decodeLiteral();
  Literal decodeLiteral(LiteralIndex index);
  void decodeArguments();
  bool decodeCBCOpcode();
  void processPut();
  void decodeGroupOpcode();

private:
  Bytecode *byte_code_;
  Stack *stack_snapshot_;
  Opcode cbc_opcode_;
  Argument argument_;
  ValueRef string_literal_;
  ValueRef literal_value_;
  uint32_t payload_;
};

} // namespace optimizer
#endif // INST_H
