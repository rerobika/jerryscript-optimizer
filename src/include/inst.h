/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef INST_H
#define INST_H

extern "C" {
#include "vm.h"
}
#include "bytecode.h"

namespace optimizer {

extern uint16_t decode_table[];

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

enum class ResultType { IDENT, REFERENCE, STACK, BLOCK, RESULT_TYPE__COUNT };

using CBCOpcode = uint16_t;
using GroupOpcode = vm_oc_types;
using LiteralIndex = uint16_t;

class Literal {
public:
  Literal() : Literal(LiteralType::LITERAL_TYPE__COUNT, 0) {}
  Literal(LiteralType type, LiteralIndex index) : type_(type), index_(index) {}

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
    assert(type() == OperandType::STACK || type() == OperandType::STACK_STACK);
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

  ResultType result() const {
    return static_cast<ResultType>((data() >> VM_OC_PUT_RESULT_SHIFT) &
                                   VM_OC_PUT_RESULT_MASK);
  }

  bool isBackwardBrach() const {
    return (data() & static_cast<uint32_t>(OperandFlags::BACKWARD_BRANCH)) != 0;
  }

  bool isForwardBrach() const { return !isBackwardBrach(); }

private:
  uint32_t data_;
};

class Opcode {
public:
  Opcode() : Opcode(0) {}
  Opcode(CBCOpcode opcode)
      : opcode_(opcode), opcode_data_(decode_table[opcode]) {}

  auto opcode() const { return opcode_; }
  auto opcodeData() const { return opcode_data_; }

  bool isExtOpcode() const { return Opcode::isExtOpcode(opcode()); }

  static bool isExtOpcode(CBCOpcode opcode) { return opcode > CBC_END; }
  static bool isEndOpcode(CBCOpcode opcode) { return opcode == CBC_EXT_NOP; }
  static bool isExtStartOpcode(CBCOpcode opcode) {
    return opcode == CBC_EXT_OPCODE;
  }

  void toExtOpcode(CBCOpcode cbc_op) {
    assert(Opcode::isExtStartOpcode(opcode()));
    assert(!Opcode::isEndOpcode(cbc_op));
    opcode_data_ = decode_table[cbc_op + CBC_END + 1];
    opcode_ = cbc_op + 256;
  }

private:
  CBCOpcode opcode_;
  OpcodeData opcode_data_;
};

class Inst {
public:
  Inst(Bytecode *byte_code) : byte_code_(byte_code), stack_snapshot_(nullptr) {}

  ~Inst() { delete stack_snapshot_; }

  auto byteCode() const { return byte_code_; }
  auto opcode() const { return opcode_; }
  auto argument() const { return argument_; }
  auto stackSnapshot() const { return stack_snapshot_; }
  auto &stack() { return byteCode()->stack(); }

  LiteralIndex decodeLiteralIndex();
  Literal decodeLiteral();
  void decodeArguments();
  bool decodeCBCOpcode();
  void decodeGroupOpcode();

private:
  Bytecode *byte_code_;
  Stack *stack_snapshot_;
  Opcode opcode_;
  Argument argument_;
};

} // namespace optimizer
#endif // INST_H
