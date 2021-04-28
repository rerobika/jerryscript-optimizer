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

  void setIndex(LiteralIndex index) { index_ = index; }

  void moveIndex(int32_t offset) {
    LOG("MOVE lit index: " << index_ << " to : " << index_ + offset);

    index_ = static_cast<LiteralIndex>(index_ + offset);
  }

  void emit(Bytecode *byte_code, std::vector<uint8_t> &buffer);

private:
  LiteralType type_;
  LiteralIndex index_;
};

class Argument {
public:
  Argument() : Argument(OperandType::OPERAND_TYPE__COUNT) {}
  Argument(OperandType type)
      : type_(type), branch_offset_(0), byte_arg_(UINT32_MAX) {}

  auto branchOffset() const { return branch_offset_; }
  auto type() const { return type_; }
  auto stackDelta() const { return stack_delta_; }
  auto lineInfo() const { return line_info_; }
  uint8_t byteArg() const { return static_cast<uint8_t>(byte_arg_); }
  auto &literals() { return literals_; }

  bool hasByteArg() const { return byte_arg_ != UINT32_MAX; }

  void setBranchOffset(int32_t offset) {
    assert(type() == OperandType::BRANCH);
    branch_offset_ = offset;
  }

  void setStackDelta(int32_t delta) {
    assert(type() == OperandType::STACK || type() == OperandType::STACK_STACK ||
           type() == OperandType::STACK_LITERAL);
    stack_delta_ = delta;
  }

  void setLineInfo(uint32_t line) { line_info_ = line; }
  void setByteArg(uint8_t byte) { byte_arg_ = byte; }

  void addLiteral(Literal &literal) { literals_.push_back(literal); }

  bool isLiteral() const { return type() >= OperandType::LITERAL; }
  bool isLiteralLiteral() const {
    return type() == OperandType::LITERAL_LITERAL;
  }

  void emit(Bytecode *byte_code, std::vector<uint8_t> &buffer);

private:
  OperandType type_;
  int32_t branch_offset_;
  uint32_t line_info_;
  uint32_t byte_arg_;
  int32_t stack_delta_;
  std::vector<Literal> literals_;
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
      : cbc_opcode_(opcode), opcode_data_(decode_table[opcode]) {
#ifndef NDEBUG
    if (!isExtOpcode()) {
      cbc_op_ = static_cast<cbc_opcode_t>(cbc_opcode_);
      cbc_ext_op_ = CBC_EXT_NOP;
      group_op_ = static_cast<vm_oc_types>(opcode_data_.groupOpcode());
    }
#endif /* !NDEBUG */
  }

  auto CBCopcode() const { return cbc_opcode_; }
  auto opcodeData() const { return opcode_data_; }

  bool isExt(CBCOpcode opcode) { return cbc_opcode_ == opcode + 256; }

  bool is(CBCOpcode opcode) {
    if (isExtOpcode()) {
      opcode += 256;
    }
    return cbc_opcode_ == opcode;
  }

  void emit(std::vector<uint8_t> &buffer) {
    if (isExtOpcode()) {
      buffer.push_back(CBC_EXT_OPCODE);
      buffer.push_back(cbc_opcode_ - 256);
    } else {
      buffer.push_back(cbc_opcode_);
    }
  }

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

#ifndef NDEBUG
    if (isExtOpcode()) {
      cbc_ext_op_ = static_cast<cbc_ext_opcode_t>(cbc_op);
      cbc_op_ = CBC_END;
      group_op_ = static_cast<vm_oc_types>(opcode_data_.groupOpcode());
    }
#endif /* !NDEBUG */
  }

private:
#ifndef NDEBUG
  cbc_opcode_t cbc_op_;
  cbc_ext_opcode_t cbc_ext_op_;
  vm_oc_types group_op_;
#endif /* !NDEBUG */
  CBCOpcode cbc_opcode_;
  OpcodeData opcode_data_;
};

enum class InstFlags {
  NONE = 0,
  JUMP = (1 << 0),
  CONDITIONAL_JUMP = (1 << 1),
  TRY_START = (1 << 2),
  TRY_CATCH = (1 << 3),
  TRY_FINALLY = (1 << 4),
  WRITE_REG = (1 << 5),
  READ_REG = (1 << 6),
  DEAD = (1 << 7),
};

class Ins {
public:
  Ins(Bytecode *byte_code)
      : byte_code_(byte_code), stack_snapshot_(nullptr),
        string_literal_(Value::_undefined()),
        literal_value_(Value::_undefined()), flags_(0), offset_(0) {}

  ~Ins() { delete stack_snapshot_; }

  auto byteCode() { return byte_code_; }
  auto &opcode() { return opcode_; }
  auto &argument() { return argument_; }
  auto stackSnapshot() const { return stack_snapshot_; }
  auto &stack() { return byteCode()->stack(); }
  auto offset() const { return offset_; }
  auto size() const { return size_; }
  auto bb() { return bb_; }
  auto &readRegs() { return read_regs_; }
  auto &writeReg() { return write_reg_; }

  bool isJump() const { return hasFlag(InstFlags::JUMP); }
  bool isConditionalJump() const {
    return hasFlag(InstFlags::CONDITIONAL_JUMP);
  }
  bool isTryContext() const {
    return isTryStart() || isTryCatch() || isTryFinally();
  }
  bool isTryStart() const { return hasFlag(InstFlags::TRY_START); }
  bool isTryCatch() const { return hasFlag(InstFlags::TRY_CATCH); }
  bool isTryFinally() const { return hasFlag(InstFlags::TRY_FINALLY); }

  bool hasFlag(InstFlags flag) const {
    return (flags_ & static_cast<uint32_t>(flag)) != 0;
  }

  void addFlag(InstFlags flag) { flags_ |= static_cast<uint32_t>(flag); }

  int32_t jumpOffset() const {
    assert(isJump());
    return argument_.branchOffset();
  }

  int32_t jumpTarget() const {
    assert(isJump());
    return offset() + jumpOffset();
  }

  bool hasNext(Bytecode *bytecode) {
    return offset() + size() <= bytecode->instructions().back()->offset();
  }

  Ins *nextInst(Bytecode *bytecode) {
    return bytecode->offsetToInst().find(offset() + size())->second;
  }

  Ins *prevInst(Bytecode *bytecode) {
    int32_t prev_offset = offset() - 1;

    while (true) {
      auto res = bytecode->offsetToInst().find(prev_offset);

      if (res != bytecode->offsetToInst().end()) {
        return res->second;
      }

      prev_offset--;
    }
  }

  void setStringLiteral() {
    Literal literal = decodeStringLiteral();
    argument_.addLiteral(literal);
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
  void setOffset(int32_t offset) {
    offset_ = offset;
    if (byteCode()->instructions().size() > 1) {
      auto &prev_inst =
          byteCode()->instructions()[byteCode()->instructions().size() - 2];
      prev_inst->setSize(offset - prev_inst->offset());
    }
  }

  void setSize(size_t size) { size_ = size; }

  void setBasicBlock(BasicBlock *bb) { bb_ = bb; }

  LiteralIndex decodeLiteralIndex();
  Literal decodeTemplateLiteral();
  Literal decodeStringLiteral();
  Literal decodeLiteral();
  Literal decodeLiteral(LiteralIndex index);
  void decodeArguments();
  bool decodeCBCOpcode();
  void processPut();
  void decodeGroupOpcode();

  void setWriteReg(uint32_t index) {
    addFlag(InstFlags::WRITE_REG);
    write_reg_ = index;
    decodeLiteral(index);
  }

  void addReadReg(uint32_t index) {
    addFlag(InstFlags::READ_REG);
    read_regs_.push_back(index);
    decodeLiteral(index);
  }

  friend std::ostream &operator<<(std::ostream &os, const Ins &inst) {
    if (inst.hasFlag(InstFlags::DEAD)) {
      os << "<dead>";
    }

    os << "Offset: " << inst.offset_ << ": "
       << cbc_names[inst.opcode_.isExtOpcode()
                        ? inst.opcode_.CBCopcode() - 256 + CBC_END + 1
                        : inst.opcode_.CBCopcode()];

    if (inst.hasFlag(InstFlags::READ_REG)) {
      os << " read: ";
      for (auto reg : inst.read_regs_) {
        os << reg << ", ";
      }
    }

    if (inst.hasFlag(InstFlags::WRITE_REG)) {
      os << " write: " << inst.write_reg_ << " ";
    }

    if (inst.argument_.type() == OperandType::BRANCH) {
      os << " offset: " << inst.argument_.branchOffset() << "(->"
         << inst.offset_ + inst.argument_.branchOffset() << ")";
    }

    if (!const_cast<Ins &>(inst).argument().literals().empty()) {
      os << " lits: (";
      auto &literals = const_cast<Ins &>(inst).argument().literals();
      for (auto iter = literals.begin(); iter != literals.end(); iter++) {
        os << (*iter).index();

        if (std::next(iter) != literals.end()) {
          os << ", ";
        }
      }
      os << ")";
    }

    return os;
  }

  void emit(std::vector<uint8_t> &buffer);

private:
  Bytecode *byte_code_;
  Stack *stack_snapshot_;
  Opcode opcode_;
  Argument argument_;
  ValueRef string_literal_;
  ValueRef literal_value_;
  BasicBlock *bb_;
  uint32_t payload_;
  uint32_t flags_;
  uint32_t offset_;
  uint32_t size_;
  std::vector<uint32_t> read_regs_;
  uint32_t write_reg_;
};

std::ostream &operator<<(std::ostream &os, const Ins &inst);
} // namespace optimizer
#endif // INST_H
